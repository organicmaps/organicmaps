#pragma once

#include <QObject>
#include <QStringList>

namespace sailfish
{
// Pauses the media players that are playing, e.g. while a voice instruction is spoken, and resumes them afterwards.
// Over MPRIS, which Android apps have too. The audio policy can't lower their volume: it only mutes them, which
// Android apps take as losing the audio focus for good.
class MediaPause : public QObject
{
  Q_OBJECT

public:
  explicit MediaPause(QObject * parent = nullptr) : QObject(parent) {}

  void Pause();
  // Only the players that Pause() paused.
  void Resume();

private:
  void PauseIfPlaying(QString const & service);

  bool m_paused = false;
  // Lets replies to an earlier Pause() be ignored.
  int m_pause = 0;
  QStringList m_players;
};
}  // namespace sailfish
