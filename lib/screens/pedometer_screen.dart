import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import 'package:provider/provider.dart';

import '../services/app_state.dart';

class PedometerScreen extends StatelessWidget {
  const PedometerScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AppState>();
    final history = state.history;
    return Scaffold(
      appBar: AppBar(title: const Text('Pedometer')),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            Card(
              child: ListTile(
                title: const Text('Current steps'),
                subtitle: Text(history.isEmpty ? 'No data yet' : '${history.first.steps}'),
              ),
            ),
            const SizedBox(height: 12),
            SizedBox(
              height: 220,
              child: BarChart(
                BarChartData(
                  barGroups: List.generate(history.length, (i) {
                    return BarChartGroupData(
                      x: i,
                      barRods: [BarChartRodData(toY: history[i].steps.toDouble())],
                    );
                  }),
                  titlesData: const FlTitlesData(show: false),
                  gridData: const FlGridData(show: false),
                  borderData: FlBorderData(show: false),
                ),
              ),
            ),
            const SizedBox(height: 12),
            ElevatedButton(
              onPressed: () {},
              child: const Text('Reset Steps (wire to BLE command)'),
            ),
          ],
        ),
      ),
    );
  }
}
