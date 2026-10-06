#pragma once

#include <QMediaPlayer>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

#include <map>
#include <string>
#include <utility>
#include <vector>

class QDBusPendingCallWatcher;

namespace sailfish
{
#ifdef OMIM_SAILFISH_HARBOUR
// The Harbour sandbox lets the app neither call Speech Note nor run speech synthesizers.
bool constexpr kVoiceSupported = false;
#else
bool constexpr kVoiceSupported = true;
#endif

// Speaks turn notifications. Sailfish OS has no speech engine, so this uses Speech Note over D-Bus when it has a
// voice for the language, else an installed speech synthesizer program, as Pure Maps does.
class VoiceGuide : public QObject
{
  Q_OBJECT

public:
  struct Engine;

  explicit VoiceGuide(QObject * parent = nullptr);
  ~VoiceGuide() override;

  bool IsAvailable() const { return !m_language.empty(); }
  // Empty without a voice.
  std::string const & Language() const { return m_language; }
  // Turn notification languages with a voice, in the core order.
  std::vector<std::string> Languages() const;
  bool IsSpeechNoteInstalled() const { return m_speechNoteInstalled; }
  bool HasSpeechNoteVoice(std::string const & language) const;
  // {id, name} of the spoken language; an empty id is its default voice.
  std::vector<std::pair<QString, QString>> SpeechNoteVoices() const;
  void SetSpeechNoteVoice(QString const & id);

  // Falls back to the app language, any Speech Note voice, or English.
  void SetPreferredLanguage(std::string const & preferred, std::string const & appLanguage);
  // changed() follows.
  void Refresh();
  void OpenSpeechNote();

  // 0..100.
  void SetVolume(int volume) { m_player.setVolume(volume); }

  // Replaces what is being said.
  void Speak(QStringList const & texts);
  void Stop();

signals:
  void changed();

private slots:
  void OnSpeechNoteFileReady(QStringList const & files, int task);

private:
  struct Program
  {
    Engine const * m_engine;
    QString m_path;
  };

  void ChooseLanguage();
  void OnSpeechNoteLanguages(QDBusPendingCallWatcher * watcher);
  void SynthesizeNext();
  void OnSynthesized(int exitCode, QProcess::ExitStatus status);
  void OnPlayerStateChanged(QMediaPlayer::State state);

  std::string m_preferredLanguage;
  std::string m_appLanguage;
  std::string m_language;
  QStringList m_queue;

  bool m_speechNoteInstalled = false;
  // Core language -> Speech Note language with a voice.
  std::map<std::string, QString> m_speechNoteVoices;
  // Core language -> its Speech Note voices as {id, name}.
  std::map<std::string, std::vector<std::pair<QString, QString>>> m_speechNoteModels;
  // Core language -> the first program that speaks it.
  std::map<std::string, Program> m_programVoices;

  // Set when Speech Note speaks the language; the task is -1 without one.
  QString m_speechNoteLanguage;
  QString m_speechNoteVoice;
  int m_speechNoteTask = -1;
  // Lets a reply to a request that was stopped meanwhile be ignored.
  int m_speechNoteRequest = 0;
  bool m_speechNotePending = false;
  // Stopped requests whose reply hasn't come yet.
  int m_speechNoteRequestsToStop = 0;
  // The task and files of a finished signal that came before the reply with the task.
  std::pair<int, QStringList> m_earlySpeechNoteFiles{-1, {}};

  // Otherwise a program writes the WAV file. Both are played here: Speech Note has no volume control and not all
  // programs can play.
  Engine const * m_engine = nullptr;
  QString m_program;
  QString m_voice;
  QString m_wavFile;
  QProcess m_process;
  // Stop() killed it: what it wrote is not played, even when it finished just before.
  bool m_processKilled = false;
  // The Sailfish media player also acquires the audio resource, without which the policy keeps it silent.
  QMediaPlayer m_player;
};
}  // namespace sailfish
