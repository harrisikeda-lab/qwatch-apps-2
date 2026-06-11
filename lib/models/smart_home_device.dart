class SmartHomeDevice {
  final String name;
  final bool powerOn;
  final int brightness;
  final int red;
  final int green;
  final int blue;

  const SmartHomeDevice({
    required this.name,
    required this.powerOn,
    required this.brightness,
    required this.red,
    required this.green,
    required this.blue,
  });

  factory SmartHomeDevice.mock(String name) => SmartHomeDevice(
        name: name,
        powerOn: true,
        brightness: 80,
        red: 255,
        green: 120,
        blue: 30,
      );
}
