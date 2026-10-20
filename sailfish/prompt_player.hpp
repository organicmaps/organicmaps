#pragma once

#include <QObject>
#include <QString>

#include <atomic>
#include <thread>

namespace sailfish
{
// Plays a WAV file such as a voice instruction. Over PulseAudio as a "notiftone" stream, which the audio policy mixes
// over music without an audio resource: taking one makes Android apps lose their audio focus for good, so they don't
// resume.
class PromptPlayer : public QObject
{
  Q_OBJECT

public:
  explicit PromptPlayer(QObject * parent = nullptr);
  ~PromptPlayer() override;

  // 0..100.
  void SetVolume(int volume);
  // Replaces what is being played.
  void Play(QString const & wavFile);
  // No finished() follows.
  void Stop();
  bool IsPlaying() const { return m_playing; }

signals:
  // Also when the file couldn't be played.
  void finished();
  // From the playback thread, for finished().
  void played(int playback);

private:
  void OnPlayed(int playback);

  bool m_playing = false;
  std::atomic<float> m_volume{1.0f};
  std::atomic<bool> m_stop{false};
  std::thread m_thread;
  // Lets the played() of a stopped playback be ignored.
  int m_playback = 0;
};
}  // namespace sailfish
