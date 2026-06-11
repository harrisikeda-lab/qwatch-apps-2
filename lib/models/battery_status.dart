class BatteryStatus {
  final int percent;
  final int voltageMv;
  final bool charging;
  final String health;

  const BatteryStatus({
    required this.percent,
    required this.voltageMv,
    required this.charging,
    required this.health,
  });

  factory BatteryStatus.mock() => const BatteryStatus(
        percent: 84,
        voltageMv: 3950,
        charging: false,
        health: 'good',
      );
}
