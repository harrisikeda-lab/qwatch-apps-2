import 'dart:async';
import 'package:flutter/foundation.dart';

import '../models/battery_status.dart';
import '../models/calendar_event.dart';
import '../models/device_settings.dart';
import '../models/music_status.dart';
import '../models/notification_item.dart';
import '../models/pedometer_entry.dart';
import '../models/qwatch_device.dart';
import '../models/smart_home_device.dart';
import '../models/weather_data.dart';
import 'ble_manager.dart';
import 'calendar_service.dart';
import 'weather_service.dart';
import 'music_service.dart';
import 'smart_home_service.dart';

class AppState extends ChangeNotifier {
  final BleManager ble = BleManager();
  final WeatherService weatherService = WeatherService();
  final CalendarService calendarService = CalendarService();
  final MusicService musicService = MusicService();
  final SmartHomeService smartHomeService = SmartHomeService();

  bool bootstrapped = false;
  bool connected = false;
  String status = 'Idle';
  QwatchDevice? selectedDevice;
  BatteryStatus battery = BatteryStatus.mock();
  WeatherData weather = WeatherData.mock();
  DeviceSettings settings = DeviceSettings.defaults();
  MusicStatus music = MusicStatus.mock();
  List<PedometerEntry> history = [];
  List<NotificationItem> notifications = [];
  List<CalendarEvent> calendarEvents = [];
  List<SmartHomeDevice> smartHomeDevices = [];

  Future<void> bootstrap() async {
    if (bootstrapped) return;
    bootstrapped = true;
    weather = await weatherService.getWeather();
    calendarEvents = await calendarService.upcomingEvents();
    music = await musicService.nowPlaying();
    smartHomeDevices = smartHomeService.seedDevices();
    notifications = [
      NotificationItem(
        title: 'Ready',
        body: 'QWatch Companion started',
        timestamp: DateTime.now(),
      ),
    ];
    notifyListeners();
  }

  Future<void> connect(QwatchDevice device) async {
    status = 'Connecting...';
    selectedDevice = device;
    notifyListeners();
    await ble.connect(device);
    connected = ble.connected;
    status = ble.connected ? 'Connected' : 'Failed';
    notifyListeners();
  }

  Future<void> disconnect() async {
    await ble.disconnect();
    connected = false;
    status = 'Disconnected';
    notifyListeners();
  }

  Future<void> refreshWeather() async {
    weather = await weatherService.getWeather();
    notifyListeners();
  }

  Future<void> refreshCalendar() async {
    calendarEvents = await calendarService.upcomingEvents();
    notifyListeners();
  }

  Future<void> refreshMusic() async {
    music = await musicService.nowPlaying();
    notifyListeners();
  }

  Future<void> refreshSmartHome() async {
    smartHomeDevices = smartHomeService.seedDevices();
    notifyListeners();
  }

  Future<void> sendKeyboardText(String text, {required bool asWifiPassword}) async {
    await ble.sendKeyboardText(text, asWifiPassword: asWifiPassword);
    notifications = [
      ...notifications,
      NotificationItem(
        title: 'Keyboard sent',
        body: asWifiPassword ? 'WiFi password sent to QWatch' : 'Text sent to QWatch',
        timestamp: DateTime.now(),
      ),
    ];
    notifyListeners();
  }

  Future<void> findPhone() async {
    await ble.findPhone();
    notifications = [
      ...notifications,
      NotificationItem(
        title: 'Find Phone',
        body: 'Phone alert triggered',
        timestamp: DateTime.now(),
      ),
    ];
    notifyListeners();
  }

  Future<void> syncFromWatch() async {
    battery = await ble.readBattery();
    weather = await ble.readWeather();
    settings = await ble.readSettings();
    history = await ble.readHistory();
    notifyListeners();
  }

  Future<void> toggleLight(SmartHomeDevice device) async {
    await smartHomeService.toggle(device);
    smartHomeDevices = smartHomeService.devices;
    notifyListeners();
  }

  Future<void> setLightBrightness(SmartHomeDevice device, int brightness) async {
    await smartHomeService.setBrightness(device, brightness);
    smartHomeDevices = smartHomeService.devices;
    notifyListeners();
  }

  Future<void> setLightColor(SmartHomeDevice device, int r, int g, int b) async {
    await smartHomeService.setColor(device, r, g, b);
    smartHomeDevices = smartHomeService.devices;
    notifyListeners();
  }
}
