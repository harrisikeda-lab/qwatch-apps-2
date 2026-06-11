class MusicStatus {
  final String artist;
  final String title;
  final bool playing;

  const MusicStatus({
    required this.artist,
    required this.title,
    required this.playing,
  });

  factory MusicStatus.mock() => const MusicStatus(
        artist: 'No Music',
        title: 'Nothing playing',
        playing: false,
      );
}
