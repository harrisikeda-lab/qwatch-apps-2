import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../services/app_state.dart';

class CalendarScreen extends StatelessWidget {
  const CalendarScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AppState>();
    return Scaffold(
      appBar: AppBar(title: const Text('Calendar')),
      body: ListView.builder(
        itemCount: state.calendarEvents.length,
        itemBuilder: (_, i) {
          final e = state.calendarEvents[i];
          return Card(
            margin: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
            child: ListTile(
              leading: const Icon(Icons.event),
              title: Text(e.title),
              subtitle: Text('${e.location} · ${e.startTime}'),
            ),
          );
        },
      ),
    );
  }
}
