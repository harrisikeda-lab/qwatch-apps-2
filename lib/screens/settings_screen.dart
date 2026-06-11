import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../services/app_state.dart';
import '../models/device_settings.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({super.key});

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  final ssidController = TextEditingController();

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AppState>();
    ssidController.text = state.settings.wifiSsid;

    return Scaffold(
      appBar: AppBar(title: const Text('Settings')),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          DropdownButtonFormField<ClockMode>(
            value: state.settings.clockMode,
            items: const [
              DropdownMenuItem(value: ClockMode.digital, child: Text('Digital')),
              DropdownMenuItem(value: ClockMode.analogue, child: Text('Analogue')),
            ],
            onChanged: (_) {},
            decoration: const InputDecoration(labelText: 'Clock mode', border: OutlineInputBorder()),
          ),
          const SizedBox(height: 12),
          TextField(
            controller: ssidController,
            decoration: const InputDecoration(labelText: 'WiFi SSID', border: OutlineInputBorder()),
          ),
          const SizedBox(height: 12),
          const TextField(
            obscureText: true,
            decoration: InputDecoration(labelText: 'WiFi Password', border: OutlineInputBorder()),
          ),
          const SizedBox(height: 12),
          ElevatedButton(
            onPressed: () {},
            child: const Text('Save to Watch (wire to BLE write)'),
          ),
        ],
      ),
    );
  }
}
