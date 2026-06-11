import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../services/app_state.dart';

class WeatherScreen extends StatelessWidget {
  const WeatherScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AppState>();
    return Scaffold(
      appBar: AppBar(title: const Text('Weather')),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          Card(child: ListTile(title: const Text('Temperature'), subtitle: Text('${state.weather.temperatureC}°C'))),
          Card(child: ListTile(title: const Text('City'), subtitle: Text(state.weather.city))),
          Card(child: ListTile(title: const Text('Coordinates'), subtitle: Text('${state.weather.latitude}, ${state.weather.longitude}'))),
          const SizedBox(height: 16),
          ElevatedButton(
            onPressed: state.refreshWeather,
            child: const Text('Refresh'),
          ),
        ],
      ),
    );
  }
}
