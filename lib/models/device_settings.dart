enum ClockMode { digital, analogue }

class DeviceSettings {
  final ClockMode clockMode;
  final int digitalFaceIndex;
  final int analogueFaceIndex;
  final String wifiSsid;

  const DeviceSettings({
    required this.clockMode,
    required this.digitalFaceIndex,
    required this.analogueFaceIndex,
    required this.wifiSsid,
  });

  factory DeviceSettings.defaults() => const DeviceSettings(
        clockMode: ClockMode.digital,
        digitalFaceIndex: 0,
        analogueFaceIndex: 0,
        wifiSsid: '',
      );
}
