import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../services/app_state.dart';

class SmartHomeScreen extends StatelessWidget {
  const SmartHomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AppState>();
    return Scaffold(
      appBar: AppBar(title: const Text('Smart Home')),
      body: ListView.builder(
        itemCount: state.smartHomeDevices.length,
        itemBuilder: (_, i) {
          final d = state.smartHomeDevices[i];
          return Card(
            margin: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
            child: ListTile(
              title: Text(d.name),
              subtitle: Text('Power: ${d.powerOn ? 'On' : 'Off'} · Brightness ${d.brightness}% · RGB(${d.red},${d.green},${d.blue})'),
              trailing: Wrap(
                spacing: 8,
                children: [
                  IconButton(
                    icon: const Icon(Icons.power_settings_new),
                    onPressed: () => state.toggleLight(d),
                  ),
                  IconButton(
                    icon: const Icon(Icons.palette),
                    onPressed: () => state.setLightColor(d, 255, 80, 20),
                  ),
                ],
              ),
            ),
          );
        },
      ),
    );
  }
}
