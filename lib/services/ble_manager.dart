import 'dart:async';
import 'dart:convert';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../models/battery_status.dart';
import '../models/device_settings.dart';
import '../models/pedometer_entry.dart';
import '../models/qwatch_device.dart';
import '../models/weather_data.dart';
import '../utils/qwatch_uuids.dart';

class BleManager {
  BluetoothDevice? _device;
  bool connected = false;

  StreamSubscription<List<int>>? _batterySub;
  StreamSubscription<List<int>>? _weatherSub;

  Future<List<QwatchDevice>> scan() async {
    final results = <QwatchDevice>[];
    try {
      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 8));
      final scanResults = await FlutterBluePlus.scanResults.first;
      for (final r in scanResults) {
        final name = r.device.platformName.isNotEmpty ? r.device.platformName : r.device.remoteId.str;
        results.add(QwatchDevice(
          id: r.device.remoteId.str,
          name: name,
          rssi: r.rssi,
        ));
      }
    } catch (_) {}
    return results;
  }

  Future<void> connect(QwatchDevice qwatch) async {
    try {
      final device = BluetoothDevice.fromId(qwatch.id);
      _device = device;
      await device.connect(timeout: const Duration(seconds: 20), autoConnect: false);
      connected = true;
      await device.requestMtu(247);
      await _discoverAndSubscribe();
    } catch (_) {
      connected = false;
      rethrow;
    }
  }

  Future<void> _discoverAndSubscribe() async {
    if (_device == null) return;
    try {
      final services = await _device!.discoverServices();
      for (final s in services) {
        for (final c in s.characteristics) {
          if (c.uuid.str.toUpperCase() == QwatchUuids.batteryLevel.toUpperCase()) {
            await c.setNotifyValue(true);
            _batterySub = c.onValueReceived.listen((v) {});
          }
          if (c.uuid.str.toUpperCase() == QwatchUuids.temperature.toUpperCase()) {
            await c.setNotifyValue(true);
            _weatherSub = c.onValueReceived.listen((v) {});
          }
        }
      }
    } catch (_) {}
  }

  Future<void> disconnect() async {
    try {
      await _batterySub?.cancel();
      await _weatherSub?.cancel();
      await _device?.disconnect();
    } catch (_) {}
    connected = false;
  }

  Future<void> sendKeyboardText(String text, {required bool asWifiPassword}) async {
    await _writeJson(
      QwatchUuids.settingsService,
      asWifiPassword ? QwatchUuids.wifiCredentials : QwatchUuids.settingsSync,
      {
        'type': asWifiPassword ? 'keyboard_text' : 'text',
        'value': text,
      },
    );
  }

  Future<void> findPhone() async {
    await _writeJson(
      QwatchUuids.settingsService,
      QwatchUuids.settingsSync,
      {'type': 'find_phone', 'trigger': true},
    );
  }

  Future<BatteryStatus> readBattery() async => BatteryStatus.mock();
  Future<WeatherData> readWeather() async => WeatherData.mock();
  Future<DeviceSettings> readSettings() async => DeviceSettings.defaults();
  Future<List<PedometerEntry>> readHistory() async => [
        PedometerEntry(date: DateTime.now(), steps: 8432, calories: 312),
        PedometerEntry(date: DateTime.now().subtract(const Duration(days: 1)), steps: 6521, calories: 245),
      ];

  Future<void> _writeJson(String serviceUuid, String charUuid, Map<String, dynamic> body) async {
    if (_device == null) return;
    final services = await _device!.discoverServices();
    final svc = services.where((s) => s.uuid.str.toUpperCase() == serviceUuid.toUpperCase()).toList();
    if (svc.isEmpty) return;
    final chars = svc.first.characteristics.where((c) => c.uuid.str.toUpperCase() == charUuid.toUpperCase()).toList();
    if (chars.isEmpty) return;
    await chars.first.write(utf8.encode(jsonEncode(body)), withoutResponse: false);
  }
}
