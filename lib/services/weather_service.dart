import '../models/weather_data.dart';

class WeatherService {
  Future<WeatherData> getWeather() async {
    return WeatherData.mock();
  }
}
