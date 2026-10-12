#include "sailfish/voice_guide.hpp"

#include "platform/languages.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QDir>
#include <QStandardPaths>

#include <algorithm>
#include <utility>

namespace sailfish
{
struct VoiceGuide::Engine
{
  char const * m_program;
  // The voice of the program for a turn notifications language, empty when it doesn't speak it.
  QString (*m_voice)(std::string const & language);
  QStringList (*m_arguments)(QString const & voice, QString const & text, QString const & wavFile);
};

namespace
{
char constexpr kSpeechNoteService[] = "org.mkiol.Speech";

QDBusMessage SpeechNoteCall(QString const & method)
{
  return QDBusMessage::createMethodCall(kSpeechNoteService, QStringLiteral("/"), kSpeechNoteService, method);
}

// Speech Note names languages like "en" or "pt_BR"; the core like "en" or "pt-BR".
QString SpeechNoteCode(std::string const & language)
{
  return QString::fromStdString(language).replace('-', '_');
}

QString EspeakVoice(std::string const & language)
{
  // Codes of the core languages that eSpeak names differently.
  if (language == "zh-Hans" || language == "zh-Hant")
    return QStringLiteral("cmn");
  if (language.starts_with("yue"))
    return QStringLiteral("yue");
  if (language == "es-MX")
    return QStringLiteral("es-419");
  return QString::fromStdString(language).toLower();
}

QString EnglishOnly(std::string const & language)
{
  return language == "en" ? QStringLiteral("en") : QString();
}

QString PicoVoice(std::string const & language)
{
  for (char const * voice : {"en-US", "de-DE", "es-ES", "fr-FR", "it-IT"})
    if (language == std::string_view(voice, 2))
      return voice;
  return {};
}

QStringList EspeakArguments(QString const & voice, QString const & text, QString const & wavFile)
{
  return {"-v", voice, "-w", wavFile, text};
}

QStringList MimicArguments(QString const &, QString const & text, QString const & wavFile)
{
  return {"-t", text, "-o", wavFile};
}

QStringList PicoArguments(QString const & voice, QString const & text, QString const & wavFile)
{
  return {"-l", voice, "-w", wavFile, text};
}

// In order of preference, as in Pure Maps.
VoiceGuide::Engine const kEngines[] = {
    {"espeak-ng", EspeakVoice, EspeakArguments}, {"espeak", EspeakVoice, EspeakArguments},
    {"mimic", EnglishOnly, MimicArguments},      {"flite", EnglishOnly, MimicArguments},
    {"pico2wave", PicoVoice, PicoArguments},
};
}  // namespace

VoiceGuide::VoiceGuide(QObject * parent) : QObject(parent)
{
  connect(&m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
          &VoiceGuide::OnSynthesized);
  // A program that doesn't start sends no finished(): go on with the next text.
  connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error)
  {
    if (error != QProcess::FailedToStart)
      return;
    LOG(LWARNING, ("Speech synthesizer didn't start:", m_program.toStdString()));
    SynthesizeNext();
  });
  connect(&m_player, &PromptPlayer::finished, this, &VoiceGuide::SynthesizeNext);
  m_resumeTimer.setSingleShot(true);
  m_resumeTimer.setInterval(1000);
  connect(&m_resumeTimer, &QTimer::timeout, &m_mediaPause, &MediaPause::Resume);
  QDBusConnection::sessionBus().connect(kSpeechNoteService, QStringLiteral("/"), kSpeechNoteService,
                                        QStringLiteral("TtsSpeechToFileFinished"), this,
                                        SLOT(OnSpeechNoteFileReady(QStringList, int)));
}

VoiceGuide::~VoiceGuide()
{
  // The QProcess destructor waits for a killed program, and its finished() must not reach the destroyed player.
  m_process.disconnect(this);
  Stop();
  m_mediaPause.Resume();
}

std::vector<std::string> VoiceGuide::Languages() const
{
  std::vector<std::string> languages;
  for (auto const & lang : routing::turns::sound::kLanguageList)
  {
    std::string const code(lang.first);
    if (m_speechNoteVoices.contains(code) || m_programVoices.contains(code))
      languages.push_back(code);
  }
  return languages;
}

bool VoiceGuide::HasSpeechNoteVoice(std::string const & language) const
{
  return m_speechNoteVoices.contains(language);
}

std::vector<std::pair<QString, QString>> VoiceGuide::SpeechNoteVoices() const
{
  auto const it = m_speechNoteModels.find(m_language);
  return it != m_speechNoteModels.end() && !m_speechNoteLanguage.isEmpty() ? it->second
                                                                           : std::vector<std::pair<QString, QString>>();
}

void VoiceGuide::SetSpeechNoteVoice(QString const & id)
{
  // An unknown voice, e.g. one deleted in Speech Note, falls back to the default of the language.
  auto const voices = SpeechNoteVoices();
  bool const known = std::any_of(voices.begin(), voices.end(), [&id](auto const & voice) { return voice.first == id; });
  m_speechNoteVoice = known ? id : QString();
}

void VoiceGuide::SetPreferredLanguage(std::string const & preferred, std::string const & appLanguage)
{
  m_preferredLanguage = preferred;
  m_appLanguage = appLanguage;
  ChooseLanguage();
}

void VoiceGuide::Refresh()
{
  if (!kVoiceSupported)
    return;

  std::vector<Program> installed;
  for (auto const & engine : kEngines)
    if (auto path = QStandardPaths::findExecutable(engine.m_program); !path.isEmpty())
      installed.push_back({&engine, std::move(path)});

  m_programVoices.clear();
  for (auto const & lang : routing::turns::sound::kLanguageList)
  {
    std::string const code(lang.first);
    for (auto const & program : installed)
    {
      if (!program.m_engine->m_voice(code).isEmpty())
      {
        m_programVoices.emplace(code, program);
        break;
      }
    }
  }
  ChooseLanguage();

  // Asks an installed Speech Note for its voices; this starts its service. Not installed, the call fails.
  auto const watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(
      QDBusMessage::createMethodCall(kSpeechNoteService, QStringLiteral("/"),
                                     QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"))
      << kSpeechNoteService << QStringLiteral("TtsModels")));
  connect(watcher, &QDBusPendingCallWatcher::finished, this, &VoiceGuide::OnSpeechNoteLanguages);
}

void VoiceGuide::OnSpeechNoteLanguages(QDBusPendingCallWatcher * watcher)
{
  watcher->deleteLater();
  QDBusPendingReply<QDBusVariant> const reply = *watcher;
  m_speechNoteVoices.clear();
  m_speechNoteModels.clear();
  m_speechNoteInstalled = !reply.isError();
  if (reply.isError())
  {
    LOG(LINFO, ("No Speech Note:", reply.error().message().toStdString()));
  }
  else
  {
    // Voices by id as [id, name, ..., language], the name like "Svenska (Piper Alma Medium Female) / sv".
    QVariantMap models;
    reply.value().variant().value<QDBusArgument>() >> models;
    std::map<QString, std::vector<std::pair<QString, QString>>> voicesByLanguage;
    for (auto const & model : models)
    {
      QStringList fields;
      if (model.canConvert<QDBusArgument>())
        model.value<QDBusArgument>() >> fields;
      else
        fields = model.toStringList();
      if (fields.size() < 4)
        continue;
      auto name = fields[1];
      if (auto const open = name.indexOf('('), close = name.lastIndexOf(')'); open >= 0 && close > open)
        name = name.mid(open + 1, close - open - 1);
      voicesByLanguage[fields[3]].emplace_back(fields[0], name);
    }
    for (auto const & lang : routing::turns::sound::kLanguageList)
    {
      std::string const code(lang.first);
      auto const speechNoteCode = SpeechNoteCode(code);
      if (auto const it = voicesByLanguage.find(speechNoteCode); it != voicesByLanguage.end())
      {
        m_speechNoteVoices.emplace(code, speechNoteCode);
        m_speechNoteModels.emplace(code, it->second);
      }
    }
  }
  ChooseLanguage();
}

void VoiceGuide::ChooseLanguage()
{
  Stop();
  auto const languages = Languages();
  auto const has = [&languages](std::string const & code) { return base::IsExist(languages, code); };
  std::string language;
  if (has(m_preferredLanguage))
    language = m_preferredLanguage;
  else if (has(m_appLanguage))
    language = m_appLanguage;
  else if (!m_speechNoteVoices.empty())
    language = m_speechNoteVoices.begin()->first;
  else if (has("en"))
    language = "en";

  m_language = language;
  m_speechNoteLanguage.clear();
  m_speechNoteVoice.clear();
  m_engine = nullptr;
  if (auto const it = m_speechNoteVoices.find(language); it != m_speechNoteVoices.end())
  {
    m_speechNoteLanguage = it->second;
    LOG(LINFO, ("Voice instructions in", language, "with Speech Note"));
  }
  else if (auto const it = m_programVoices.find(language); it != m_programVoices.end())
  {
    m_engine = it->second.m_engine;
    m_program = it->second.m_path;
    m_voice = m_engine->m_voice(language);
    LOG(LINFO, ("Voice instructions in", language, "with", m_program.toStdString()));
  }
  else
  {
    LOG(LINFO, ("No voice for voice instructions"));
  }
  emit changed();
}

void VoiceGuide::OpenSpeechNote()
{
  QDBusConnection::sessionBus().asyncCall(
      QDBusMessage::createMethodCall(QStringLiteral("org.mkiol.dsnote"), QStringLiteral("/org/mkiol/dsnote"),
                                     QStringLiteral("org.freedesktop.Application"), QStringLiteral("Activate"))
      << QVariantMap{});
}

void VoiceGuide::Speak(QStringList const & texts)
{
  if (!IsAvailable() || texts.isEmpty())
    return;
  Stop();
  m_resumeTimer.stop();
  m_mediaPause.Pause();
  m_queue = texts;
  SynthesizeNext();
}

void VoiceGuide::Stop()
{
  m_queue.clear();
  // Its task is only known from the reply, which stops it: Speech Note refuses new speech until then.
  if (m_speechNotePending && m_speechNoteTask < 0)
    ++m_speechNoteRequestsToStop;
  ++m_speechNoteRequest;
  m_speechNotePending = false;
  m_earlySpeechNoteFiles = {-1, {}};
  if (m_speechNoteTask >= 0)
  {
    QDBusConnection::sessionBus().asyncCall(SpeechNoteCall(QStringLiteral("TtsStopSpeech")) << m_speechNoteTask);
    m_speechNoteTask = -1;
  }
  // Not waited for: its finished() starts the next speech then.
  if (m_process.state() != QProcess::NotRunning)
  {
    m_process.kill();
    m_processKilled = true;
  }
  m_player.Stop();
  m_resumeTimer.start();
}

void VoiceGuide::OnSpeechNoteFileReady(QStringList const & files, int task)
{
  // The signal can come before the reply that tells the task: kept for it.
  if (m_speechNotePending && m_speechNoteTask < 0)
  {
    m_earlySpeechNoteFiles = {task, files};
    return;
  }
  if (task != m_speechNoteTask)
    return;
  m_speechNoteTask = -1;
  m_speechNotePending = false;
  if (files.isEmpty())
  {
    SynthesizeNext();
    return;
  }
  m_player.Play(files.first());
}

void VoiceGuide::SynthesizeNext()
{
  if (m_speechNotePending || m_speechNoteRequestsToStop > 0 || m_process.state() != QProcess::NotRunning ||
      m_player.IsPlaying())
    return;
  if (m_queue.isEmpty())
  {
    m_resumeTimer.start();
    return;
  }
  if (!m_speechNoteLanguage.isEmpty())
  {
    m_speechNotePending = true;
    auto const request = m_speechNoteRequest;
    auto const watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(
        SpeechNoteCall(QStringLiteral("TtsSpeechToFile"))
        << m_queue.takeFirst() << (m_speechNoteVoice.isEmpty() ? m_speechNoteLanguage : m_speechNoteVoice)
        << QVariantMap{{"audio_format", "wav"}}));
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, request](QDBusPendingCallWatcher * call)
    {
      call->deleteLater();
      QDBusPendingReply<int> const reply = *call;
      if (request != m_speechNoteRequest)
      {
        if (!reply.isError() && reply.value() >= 0)
          QDBusConnection::sessionBus().asyncCall(SpeechNoteCall(QStringLiteral("TtsStopSpeech")) << reply.value());
        --m_speechNoteRequestsToStop;
        SynthesizeNext();
        return;
      }
      // -1 when Speech Note is busy with something else.
      if (reply.isError() || reply.value() < 0)
      {
        LOG(LWARNING, ("Speech Note didn't take the voice instruction:", reply.error().message().toStdString()));
        m_speechNotePending = false;
        SynthesizeNext();
        return;
      }
      m_speechNoteTask = reply.value();
      auto const early = std::exchange(m_earlySpeechNoteFiles, {-1, {}});
      if (early.first == m_speechNoteTask)
        OnSpeechNoteFileReady(early.second, early.first);
    });
    return;
  }
  // Two files take turns, so that a new one never replaces the media being played.
  QString const dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  QDir().mkpath(dir);
  m_wavFile = dir + (m_wavFile.endsWith("speech0.wav") ? "/speech1.wav" : "/speech0.wav");
  m_process.start(m_program, m_engine->m_arguments(m_voice, m_queue.takeFirst(), m_wavFile));
}

void VoiceGuide::OnSynthesized(int exitCode, QProcess::ExitStatus status)
{
  if (std::exchange(m_processKilled, false))
  {
    SynthesizeNext();
    return;
  }
  if (status != QProcess::NormalExit || exitCode != 0)
  {
    LOG(LWARNING, ("Speech synthesizer failed:", m_program.toStdString(), exitCode,
                   m_process.readAllStandardError().toStdString()));
    SynthesizeNext();
    return;
  }
  m_player.Play(m_wavFile);
}
}  // namespace sailfish
