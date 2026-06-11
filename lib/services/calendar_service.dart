import '../models/calendar_event.dart';

class CalendarService {
  Future<List<CalendarEvent>> upcomingEvents() async {
    return [
      CalendarEvent(
        title: 'Math Exam',
        startTime: DateTime.now().add(const Duration(hours: 3)),
        location: 'School',
      ),
      CalendarEvent(
        title: 'Project Meeting',
        startTime: DateTime.now().add(const Duration(days: 1)),
        location: 'Online',
      ),
    ];
  }
}
