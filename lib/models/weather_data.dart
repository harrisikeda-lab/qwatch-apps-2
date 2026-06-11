class WeatherData {
  final int temperatureC;
  final int weatherCode;
  final String city;
  final double latitude;
  final double longitude;

  const WeatherData({
    required this.temperatureC,
    required this.weatherCode,
    required this.city,
    required this.latitude,
    required this.longitude,
  });

  factory WeatherData.mock() => const WeatherData(
        temperatureC: 22,
        weatherCode: 0,
        city: 'Unknown',
        latitude: 0,
        longitude: 0,
      );
}
