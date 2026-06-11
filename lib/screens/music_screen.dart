import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../services/app_state.dart';

class MusicScreen extends StatelessWidget {
  const MusicScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AppState>();
    return Scaffold(
      appBar: AppBar(title: const Text('Music')),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          Card(child: ListTile(title: const Text('Artist'), subtitle: Text(state.music.artist))),
          Card(child: ListTile(title: const Text('Title'), subtitle: Text(state.music.title))),
          Card(child: ListTile(title: const Text('Playing'), subtitle: Text(state.music.playing ? 'Yes' : 'No'))),
          const SizedBox(height: 16),
          ElevatedButton(onPressed: state.refreshMusic, child: const Text('Refresh')),
        ],
      ),
    );
  }
}
