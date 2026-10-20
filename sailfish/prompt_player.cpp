#include "sailfish/prompt_player.hpp"

#include "base/logging.hpp"

#include <QFile>
#include <QtEndian>

#include <pulse/error.h>
#include <pulse/simple.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

namespace sailfish
{
namespace
{
// Matches the media.name rule of the nonsilent group in /etc/pulse/xpolicy.conf.
char constexpr kStreamName[] = "notiftone";

struct Wav
{
  uint32_t m_rate = 0;
  uint8_t m_channels = 0;
  // Interleaved, -1..1.
  std::vector<float> m_samples;
};

// format is 1 for PCM, 3 for float.
float ReadSample(uchar const * sample, uint16_t format, size_t bytesPerSample)
{
  float value;
  if (format == 3)
  {
    quint32 const raw = qFromLittleEndian<quint32>(sample);
    std::memcpy(&value, &raw, sizeof(value));
  }
  else if (bytesPerSample == 1)
  {
    value = (sample[0] - 128) / 128.0f;
  }
  else
  {
    // The sample into the top bytes of a 32-bit integer, which keeps its sign.
    uint32_t raw = 0;
    for (size_t b = 0; b < bytesPerSample; ++b)
      raw |= static_cast<uint32_t>(sample[b]) << (8 * (4 - bytesPerSample + b));
    value = static_cast<int32_t>(raw) / 2147483648.0f;
  }
  return value;
}

// The data chunk, after the fmt one.
bool ReadSamples(uchar const * chunk, size_t chunkSize, uint16_t format, uint16_t bits, Wav & wav)
{
  if (wav.m_channels == 0 || wav.m_rate == 0)
    return false;
  size_t const bytesPerSample = bits / 8;
  if (!((format == 1 && bytesPerSample >= 1 && bytesPerSample <= 4) || (format == 3 && bytesPerSample == 4)))
    return false;
  size_t const count = chunkSize / bytesPerSample;
  wav.m_samples.resize(count);
  for (size_t i = 0; i < count; ++i)
    wav.m_samples[i] = ReadSample(chunk + i * bytesPerSample, format, bytesPerSample);
  return true;
}

// PCM or float WAV, as speech synthesizers write.
bool ReadWav(QString const & path, Wav & wav)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return false;
  QByteArray const bytes = file.readAll();
  auto const data = reinterpret_cast<uchar const *>(bytes.constData());
  auto const size = static_cast<size_t>(bytes.size());
  if (size < 12 || std::memcmp(data, "RIFF", 4) != 0 || std::memcmp(data + 8, "WAVE", 4) != 0)
    return false;

  uint16_t format = 0;
  uint16_t bits = 0;
  for (size_t pos = 12; pos + 8 <= size;)
  {
    size_t chunkSize = std::min<size_t>(qFromLittleEndian<quint32>(data + pos + 4), size - pos - 8);
    // Programs that write to a pipe leave the data size unset: it then runs to the end.
    if (chunkSize == 0 && std::memcmp(data + pos, "data", 4) == 0)
      chunkSize = size - pos - 8;
    uchar const * chunk = data + pos + 8;
    if (std::memcmp(data + pos, "fmt ", 4) == 0 && chunkSize >= 16)
    {
      format = qFromLittleEndian<quint16>(chunk);
      wav.m_channels = static_cast<uint8_t>(qFromLittleEndian<quint16>(chunk + 2));
      wav.m_rate = qFromLittleEndian<quint32>(chunk + 4);
      bits = qFromLittleEndian<quint16>(chunk + 14);
      // WAVE_FORMAT_EXTENSIBLE: the format is the start of the subformat GUID.
      if (format == 0xFFFE && chunkSize >= 26)
        format = qFromLittleEndian<quint16>(chunk + 24);
    }
    else if (std::memcmp(data + pos, "data", 4) == 0)
    {
      return ReadSamples(chunk, chunkSize, format, bits, wav);
    }
    pos += 8 + chunkSize + (chunkSize & 1);
  }
  return false;
}
}  // namespace

PromptPlayer::PromptPlayer(QObject * parent) : QObject(parent)
{
  connect(this, &PromptPlayer::played, this, &PromptPlayer::OnPlayed, Qt::QueuedConnection);
}

PromptPlayer::~PromptPlayer()
{
  Stop();
}

void PromptPlayer::SetVolume(int volume)
{
  m_volume = std::clamp(volume, 0, 100) / 100.0f;
}

void PromptPlayer::Play(QString const & wavFile)
{
  Stop();
  m_playing = true;
  m_stop = false;
  auto const playback = ++m_playback;
  Wav wav;
  if (!ReadWav(wavFile, wav))
  {
    LOG(LWARNING, ("Can't play", wavFile.toStdString()));
    emit played(playback);
    return;
  }
  m_thread = std::thread([this, playback, wav = std::move(wav)]() mutable
  {
    pa_sample_spec const spec{PA_SAMPLE_FLOAT32LE, wav.m_rate, wav.m_channels};
    // A short buffer, so that a stopped prompt goes silent at once.
    pa_buffer_attr attr;
    std::memset(&attr, 0xFF, sizeof(attr));
    attr.tlength = static_cast<uint32_t>(pa_usec_to_bytes(100 * 1000, &spec));
    int error = 0;
    pa_simple * stream =
        pa_simple_new(nullptr, "Organic Maps", PA_STREAM_PLAYBACK, nullptr, kStreamName, &spec, nullptr, &attr, &error);
    if (!stream)
    {
      LOG(LWARNING, ("Can't open an audio stream:", pa_strerror(error)));
      emit played(playback);
      return;
    }
    // 50 ms at a time, to see Stop() and volume changes.
    size_t const chunk = std::max<size_t>(1, wav.m_rate / 20) * wav.m_channels;
    std::vector<float> buffer(chunk);
    for (size_t pos = 0; pos < wav.m_samples.size() && !m_stop; pos += chunk)
    {
      size_t const count = std::min(chunk, wav.m_samples.size() - pos);
      float const volume = m_volume;
      for (size_t i = 0; i < count; ++i)
        buffer[i] = wav.m_samples[pos + i] * volume;
      if (pa_simple_write(stream, buffer.data(), count * sizeof(float), &error) < 0)
      {
        LOG(LWARNING, ("Audio playback failed:", pa_strerror(error)));
        break;
      }
    }
    if (m_stop)
      pa_simple_flush(stream, &error);
    else
      pa_simple_drain(stream, &error);
    pa_simple_free(stream);
    emit played(playback);
  });
}

void PromptPlayer::Stop()
{
  m_playing = false;
  m_stop = true;
  if (m_thread.joinable())
    m_thread.join();
}

void PromptPlayer::OnPlayed(int playback)
{
  if (playback != m_playback || !m_playing)
    return;
  if (m_thread.joinable())
    m_thread.join();
  m_playing = false;
  emit finished();
}
}  // namespace sailfish
