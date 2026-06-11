import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../services/app_state.dart';
import '../widgets/status_card.dart';
import '../widgets/action_tile.dart';

class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AppState>();
    return Scaffold(
      appBar: AppBar(
        title: const Text('QWatch Companion'),
        actions: [
          IconButton(
            icon: const Icon(Icons.bluetooth_searching),
            onPressed: () => Navigator.pushNamed(context, '/discover'),
          ),
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: state.syncFromWatch,
          ),
        ],
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          StatusCard(
            title: state.connected ? 'Watch connected' : 'Watch disconnected',
            subtitle: state.selectedDevice?.name ?? state.status,
            icon: state.connected ? Icons.watch : Icons.watch_off,
          ),
          const SizedBox(height: 12),
          StatusCard(
            title: 'Battery',
            subtitle: '${state.battery.percent}% · ${state.battery.voltageMv}mV · ${state.battery.health}',
            icon: Icons.battery_full,
          ),
          StatusCard(
            title: 'Weather',
            subtitle: '${state.weather.temperatureC}°C · ${state.weather.city}',
            icon: Icons.wb_sunny,
          ),
          StatusCard(
            title: 'Music',
            subtitle: '${state.music.artist} — ${state.music.title}',
            icon: Icons.music_note,
          ),
          const SizedBox(height: 12),
          ActionTile(
            label: 'Phone Keyboard',
            icon: Icons.keyboard_alt,
            onTap: () => Navigator.pushNamed(context, '/keyboard'),
          ),
          ActionTile(
            label: 'Weather',
            icon: Icons.cloud,
            onTap: () => Navigator.pushNamed(context, '/weather'),
          ),
          ActionTile(
            label: 'Pedometer',
            icon: Icons.directions_walk,
            onTap: () => Navigator.pushNamed(context, '/pedometer'),
          ),
          ActionTile(
            label: 'Calendar',
            icon: Icons.calendar_month,
            onTap: () => Navigator.pushNamed(context, '/calendar'),
          ),
          ActionTile(
            label: 'Music',
            icon: Icons.music_video,
            onTap: () => Navigator.pushNamed(context, '/music'),
          ),
          ActionTile(
            label: 'Smart Home',
            icon: Icons.lightbulb,
            onTap: () => Navigator.pushNamed(context, '/smarthome'),
          ),
          ActionTile(
            label: 'Notifications',
            icon: Icons.notifications,
            onTap: () => Navigator.pushNamed(context, '/notifications'),
          ),
          ActionTile(
            label: 'Settings',
            icon: Icons.settings,
            onTap: () => Navigator.pushNamed(context, '/settings'),
          ),
        ],
      ),
      floatingActionButton: FloatingActionButton.extended(
        onPressed: state.findPhone,
        icon: const Icon(Icons.phone_android),
        label: const Text('Find Phone'),
      ),
    );
  }
}
