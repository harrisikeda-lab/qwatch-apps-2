import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import 'services/app_state.dart';
import 'screens/home_screen.dart';
import 'screens/device_discovery_screen.dart';
import 'screens/keyboard_screen.dart';
import 'screens/weather_screen.dart';
import 'screens/pedometer_screen.dart';
import 'screens/settings_screen.dart';
import 'screens/calendar_screen.dart';
import 'screens/music_screen.dart';
import 'screens/smart_home_screen.dart';
import 'screens/notifications_screen.dart';

class QWatchApp extends StatelessWidget {
  const QWatchApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'QWatch Companion',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        useMaterial3: true,
        colorSchemeSeed: Colors.blue,
        scaffoldBackgroundColor: const Color(0xFFF7F8FA),
      ),
      routes: {
        '/': (_) => const HomeScreen(),
        '/discover': (_) => const DeviceDiscoveryScreen(),
        '/keyboard': (_) => const KeyboardScreen(),
        '/weather': (_) => const WeatherScreen(),
        '/pedometer': (_) => const PedometerScreen(),
        '/settings': (_) => const SettingsScreen(),
        '/calendar': (_) => const CalendarScreen(),
        '/music': (_) => const MusicScreen(),
        '/smarthome': (_) => const SmartHomeScreen(),
        '/notifications': (_) => const NotificationsScreen(),
      },
      initialRoute: '/',
    );
  }
}
