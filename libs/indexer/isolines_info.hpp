#pragma once

#include "indexer/mwm_set.hpp"

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include <cstdint>

class DataSource;

namespace isolines
{
enum class Quality
{
  None,
  Poor,
  Normal,
};

struct IsolinesInfo
{
  IsolinesInfo() = default;
  IsolinesInfo(int16_t minAlitude, int16_t maxAlitude, int16_t altStep)
    : m_minAltitude(minAlitude)
    , m_maxAltitude(maxAlitude)
    , m_altStep(altStep)
  {}

  isolines::Quality GetQuality() const
  {
    if (m_altStep == 0)
      return isolines::Quality::None;

    if (m_altStep >= 500)
      return isolines::Quality::Poor;

    return isolines::Quality::Normal;
  }

  int16_t m_minAltitude = 0;
  int16_t m_maxAltitude = 0;
  int16_t m_altStep = 0;
};

enum class Version : uint8_t
{
  V0 = 0,
  Latest = V0
};

class Serializer
{
public:
  explicit Serializer(IsolinesInfo && info) : m_info(info) {}

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    WriteToSink(sink, static_cast<std::underlying_type_t<Version>>(Version::Latest));
    WriteToSink(sink, m_info.m_minAltitude);
    WriteToSink(sink, m_info.m_maxAltitude);
    WriteToSink(sink, m_info.m_altStep);
  }

private:
  IsolinesInfo m_info;
};

class Deserializer
{
public:
  explicit Deserializer(IsolinesInfo & info) : m_info(info) {}

  template <typename Reader>
  bool Deserialize(Reader & reader)
  {
    NonOwningReaderSource source(reader);
    auto const version = static_cast<Version>(ReadPrimitiveFromSource<std::underlying_type_t<Version>>(source));

    auto subReader = reader.CreateSubReader(source.Pos(), source.Size());
    CHECK(subReader, ());

    switch (version)
    {
    case Version::V0: return DeserializeV0(*subReader);
    }
    UNREACHABLE();

    return false;
  }

private:
  template <typename Reader>
  bool DeserializeV0(Reader & reader)
  {
    NonOwningReaderSource source(reader);
    m_info.m_minAltitude = ReadPrimitiveFromSource<int16_t>(source);
    m_info.m_maxAltitude = ReadPrimitiveFromSource<int16_t>(source);
    m_info.m_altStep = ReadPrimitiveFromSource<int16_t>(source);
    return true;
  }

  IsolinesInfo & m_info;
};

bool LoadIsolinesInfo(DataSource const & dataSource, MwmSet::MwmId const & mwmId, IsolinesInfo & info);
}  // namespace isolines
