class QwatchDevice {
  final String id;
  final String name;
  final int? rssi;

  const QwatchDevice({required this.id, required this.name, this.rssi});
}
