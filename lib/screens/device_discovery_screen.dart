import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../services/app_state.dart';
import '../models/qwatch_device.dart';

class DeviceDiscoveryScreen extends StatefulWidget {
  const DeviceDiscoveryScreen({super.key});

  @override
  State<DeviceDiscoveryScreen> createState() => _DeviceDiscoveryScreenState();
}

class _DeviceDiscoveryScreenState extends State<DeviceDiscoveryScreen> {
  List<QwatchDevice> devices = [];
  bool scanning = false;

  Future<void> _scan() async {
    setState(() => scanning = true);
    devices = await context.read<AppState>().ble.scan();
    setState(() => scanning = false);
  }

  @override
  void initState() {
    super.initState();
    _scan();
  }

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AppState>();
    return Scaffold(
      appBar: AppBar(title: const Text('Discover QWatch')),
      body: Column(
        children: [
          ListTile(
            title: const Text('Scan status'),
            subtitle: Text(scanning ? 'Scanning...' : 'Idle'),
            trailing: IconButton(onPressed: _scan, icon: const Icon(Icons.refresh)),
          ),
          Expanded(
            child: ListView.builder(
              itemCount: devices.length,
              itemBuilder: (_, i) {
                final d = devices[i];
                return Card(
                  margin: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
                  child: ListTile(
                    title: Text(d.name),
                    subtitle: Text('${d.id} · RSSI ${d.rssi ?? '-'}'),
                    trailing: ElevatedButton(
                      onPressed: () async {
                        await state.connect(d);
                        if (mounted) Navigator.pop(context);
                      },
                      child: const Text('Connect'),
                    ),
                  ),
                );
              },
            ),
          ),
        ],
      ),
    );
  }
}
