import '../models/music_status.dart';

class MusicService {
  Future<MusicStatus> nowPlaying() async {
    return MusicStatus.mock();
  }
}
