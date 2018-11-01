#pragma once

#include "routing/maxspeed_conversion.hpp"

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace routing
{
class MaxspeedSerializer
{
public:
  MaxspeedSerializer() = delete;

  template <class Sink>
  static void Serialize(std::vector<FeatureMaxspeed> const & speeds, Sink & sink)
  {
    CHECK(std::is_sorted(speeds.cbegin(), speeds.cend()), ());

    // @TODO(bykoianko) Now serialization is implemented in the simplest way for research purposes.
    // It should be rewrite in a better way using MaxspeedConverter before the PR is merged.
    Header header(static_cast<uint32_t>(speeds.size()));
    header.Serialize(sink);

    for (auto const & s : speeds)
    {
      WriteToSink(sink, s.GetFeatureId());
      WriteToSink(sink, static_cast<uint8_t>(s.GetUnits()));
      WriteToSink(sink, s.GetForward());
      WriteToSink(sink, s.GetBackward());
    }
  }

  template <class Source>
  static void Deserialize(Source & src, std::vector<FeatureMaxspeed> & speeds)
  {
    Header header;
    header.Deserialize(src);

    speeds.clear();
    for (size_t i = 0; i < header.GetSize(); ++i)
    {
      auto const fid = ReadPrimitiveFromSource<uint32_t>(src);
      auto const units = static_cast<measurement_utils::Units>(ReadPrimitiveFromSource<uint8_t>(src));
      auto const forwardSpeed = ReadPrimitiveFromSource<uint16_t>(src);
      auto const backwardSpeed = ReadPrimitiveFromSource<uint16_t>(src);

      speeds.emplace_back(fid, units, forwardSpeed, backwardSpeed);
    }
  }

private:
  static uint8_t constexpr kLastVersion = 0;

  class Header final
  {
  public:
    Header() = default;
    explicit Header(uint32_t size) : m_size(size) {}

    template <class Sink>
    void Serialize(Sink & sink) const
    {
      WriteToSink(sink, m_version);
      WriteToSink(sink, m_reserved);
      WriteToSink(sink, m_size);
    }

    template <class Source>
    void Deserialize(Source & src)
    {
      m_version = ReadPrimitiveFromSource<decltype(m_version)>(src);
      m_reserved = ReadPrimitiveFromSource<decltype(m_reserved)>(src);
      m_size = ReadPrimitiveFromSource<decltype(m_size)>(src);
    }

    uint32_t GetSize() const { return m_size; }

  private:
    uint8_t m_version = kLastVersion;
    uint8_t m_reserved = 0;
    uint32_t m_size = 0;
  };
};
}  // namespace routing
