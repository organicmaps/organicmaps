#pragma once

#include "coding/write_to_sink.hpp"

#include <cstdint>
#include <type_traits>

namespace isolines
{
struct IsolinesInfo
{
  IsolinesInfo() = default;
  IsolinesInfo(int16_t minAlitude, int16_t maxAlitude, int16_t altStep)
    : m_minAltitude(minAlitude)
    , m_maxAltitude(maxAlitude)
    , m_altStep(altStep)
  {}

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
}  // namespace isolines
