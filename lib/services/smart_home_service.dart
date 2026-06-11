import '../models/smart_home_device.dart';

class SmartHomeService {
  List<SmartHomeDevice> devices = [
    SmartHomeDevice.mock('Qasa Strip'),
    SmartHomeDevice.mock('Desk Lamp'),
  ];

  List<SmartHomeDevice> seedDevices() => devices;

  Future<void> toggle(SmartHomeDevice device) async {
    devices = [
      for (final d in devices)
        if (d.name == device.name)
          SmartHomeDevice(
            name: d.name,
            powerOn: !d.powerOn,
            brightness: d.brightness,
            red: d.red,
            green: d.green,
            blue: d.blue,
          )
        else
          d,
    ];
  }

  Future<void> setBrightness(SmartHomeDevice device, int brightness) async {
    devices = [
      for (final d in devices)
        if (d.name == device.name)
          SmartHomeDevice(
            name: d.name,
            powerOn: d.powerOn,
            brightness: brightness.clamp(0, 100),
            red: d.red,
            green: d.green,
            blue: d.blue,
          )
        else
          d,
    ];
  }

  Future<void> setColor(SmartHomeDevice device, int r, int g, int b) async {
    devices = [
      for (final d in devices)
        if (d.name == device.name)
          SmartHomeDevice(
            name: d.name,
            powerOn: d.powerOn,
            brightness: d.brightness,
            red: r.clamp(0, 255),
            green: g.clamp(0, 255),
            blue: b.clamp(0, 255),
          )
        else
          d,
    ];
  }
}
