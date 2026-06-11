import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../services/app_state.dart';

class KeyboardScreen extends StatefulWidget {
  const KeyboardScreen({super.key});

  @override
  State<KeyboardScreen> createState() => _KeyboardScreenState();
}

class _KeyboardScreenState extends State<KeyboardScreen> {
  final controller = TextEditingController();
  bool asWifiPassword = true;

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AppState>();
    return Scaffold(
      appBar: AppBar(title: const Text('Phone Keyboard')),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            SwitchListTile(
              title: const Text('WiFi password mode'),
              subtitle: const Text('Sends text to the watch WiFi entry screen'),
              value: asWifiPassword,
              onChanged: (v) => setState(() => asWifiPassword = v),
            ),
            TextField(
              controller: controller,
              decoration: const InputDecoration(
                labelText: 'Type here',
                border: OutlineInputBorder(),
              ),
              minLines: 3,
              maxLines: 6,
            ),
            const SizedBox(height: 16),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton(
                    onPressed: () async {
                      await state.sendKeyboardText(
                        controller.text,
                        asWifiPassword: asWifiPassword,
                      );
                      if (context.mounted) Navigator.pop(context);
                    },
                    child: const Text('Send to Watch'),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 24),
            const Text(
              'This screen is ready to bridge to the watch WiFi entry field once the BLE GATT server is present.',
            ),
          ],
        ),
      ),
    );
  }
}
