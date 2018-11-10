#pragma once

#include "routing/maxspeeds.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "coding/reader.hpp"
#include "coding/simple_dense_coding.hpp"
#include "coding/succinct_mapper.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

#include "3party/succinct/elias_fano.hpp"

namespace routing
{
void GetForwardMaxspeedStats(std::vector<FeatureMaxspeed> const & speeds,
                             size_t & forwardMaxspeedsNumber, uint32_t & maxForwardFeatureId);
void CheckSpeeds(std::vector<FeatureMaxspeed> const & speeds);

/// \brief
/// Section name: "maxspeed".
/// Description: keeping value of maxspeed tag for roads.
/// Section tables:
/// * header
/// * elias_fano with feature ids which have maxspeed tag in forward direction only
/// * SimpleDenseCoding with code of maxspeed
/// * table with vector with feature ids and maxspeeds which have maxspeed for both directions.
class MaxspeedSerializer
{
public:
  MaxspeedSerializer() = delete;

  template <class Sink>
  static void Serialize(std::vector<FeatureMaxspeed> const & speeds, Sink & sink)
  {
    CheckSpeeds(speeds);

    auto const startOffset = sink.Pos();
    Header header;
    header.Serialize(sink);

    size_t forwardNumber = 0;
    size_t serializedForwardNumber = 0;
    uint32_t maxForwardFeatureId = 0;
    GetForwardMaxspeedStats(speeds, forwardNumber, maxForwardFeatureId);

    // Keeping forward (one direction) maxspeeds.
    if (forwardNumber > 0)
    {
      succinct::elias_fano::elias_fano_builder builder(maxForwardFeatureId + 1, forwardNumber);

      std::vector<uint8_t> forwardMaxspeeds;
      forwardMaxspeeds.reserve(forwardNumber);
      for (auto const & s : speeds)
      {
        CHECK(s.IsValid(), ());
        if (s.IsBidirectional())
          continue;

        auto forwardSpeedInUnits = s.GetForwardSpeedInUnits();
        CHECK(forwardSpeedInUnits.IsValid(), ());

        auto const macro = GetMaxspeedConverter().SpeedToMacro(forwardSpeedInUnits);
        if (macro == SpeedMacro::Undefined)
          continue; // No appropriate speed for |forwardSpeedInUnits| in SpeedMacro enum.

        forwardMaxspeeds.push_back(static_cast<uint8_t>(macro));
        builder.push_back(s.GetFeatureId());
      }
      serializedForwardNumber = forwardMaxspeeds.size();

      // Table of feature ids which have maxspeed forward info and don't have maxspeed backward.
      coding::FreezeVisitor<Writer> visitor(sink);
      succinct::elias_fano(&builder).map(visitor);

      header.m_forwardMaxspeedOffset = static_cast<uint32_t>(sink.Pos() - startOffset);

      // Maxspeeds which have only forward value.
      coding::SimpleDenseCoding simpleDenseCoding(forwardMaxspeeds);
      coding::Freeze(simpleDenseCoding, sink, "ForwardMaxspeeds");
    }
    else
    {
      header.m_forwardMaxspeedOffset = static_cast<uint32_t>(sink.Pos() - startOffset);
    }
    header.m_bidirectionalMaxspeedOffset = static_cast<uint32_t>(sink.Pos() - startOffset);

    // Keeping bidirectional maxspeeds.
    for (auto const & s : speeds)
    {
      if (!s.IsBidirectional())
        continue;

      auto const forwardMacro = GetMaxspeedConverter().SpeedToMacro(s.GetForwardSpeedInUnits());
      auto const backwardMacro = GetMaxspeedConverter().SpeedToMacro(s.GetBackwardSpeedInUnits());
      if (forwardMacro == SpeedMacro::Undefined || backwardMacro == SpeedMacro::Undefined)
        continue; // No appropriate speed for |s| in SpeedMacro enum.

      ++header.m_bidirectionalMaxspeedNumber;
      WriteToSink(sink, s.GetFeatureId());
      WriteToSink(sink, static_cast<uint8_t>(forwardMacro));
      WriteToSink(sink, static_cast<uint8_t>(backwardMacro));
    }

    auto const endOffset = sink.Pos();
    sink.Seek(startOffset);
    header.Serialize(sink);
    sink.Seek(endOffset);

    LOG(LINFO, ("Serialized", serializedForwardNumber, "forward maxspeeds and",
                header.m_bidirectionalMaxspeedNumber,
                "bidirectional maxspeeds. Section size:", endOffset - startOffset, "bytes."));
  }

  template <class Source>
  static void Deserialize(Source & src, Maxspeeds & maxspeeds)
  {
    // Note. Now it's assumed that only little-endian architectures are supported.
    // (See a check at the beginning of generator_tools.) If it's nessacery to support
    // big-indian architectures code below should be modified.
    CHECK(maxspeeds.IsEmpty(), ());

    Header header;
    header.Deserialize(src);

    if (header.m_forwardMaxspeedOffset != header.m_bidirectionalMaxspeedOffset)
    {
      // Reading maxspeed information for features which have only forward maxspeed.
      size_t const forwardTableSz = header.m_forwardMaxspeedOffset - sizeof(Header);
      std::vector<uint8_t> forwardTableData(forwardTableSz);
      src.Read(forwardTableData.data(), forwardTableData.size());
      maxspeeds.m_forwardMaxspeedTableRegion = std::make_unique<CopiedMemoryRegion>(std::move(forwardTableData));
      coding::Map(maxspeeds.m_forwardMaxspeedsTable,
          maxspeeds.m_forwardMaxspeedTableRegion->ImmutableData(), "ForwardMaxspeedsTable");

      size_t const forwardSz = header.m_bidirectionalMaxspeedOffset - header.m_forwardMaxspeedOffset;
      std::vector<uint8_t> forwardData(forwardSz);
      src.Read(forwardData.data(), forwardData.size());
      maxspeeds.m_forwardMaxspeedRegion = std::make_unique<CopiedMemoryRegion>(std::move(forwardData));
      Map(maxspeeds.m_forwardMaxspeeds, maxspeeds.m_forwardMaxspeedRegion->ImmutableData(), "ForwardMaxspeeds");
    }

    maxspeeds.m_bidirectionalMaxspeeds.reserve(header.m_bidirectionalMaxspeedNumber);
    for (size_t i = 0; i < header.m_bidirectionalMaxspeedNumber; ++i)
    {
      auto const fid = ReadPrimitiveFromSource<uint32_t>(src);
      auto const forward = ReadPrimitiveFromSource<uint8_t>(src);
      auto const backward = ReadPrimitiveFromSource<uint8_t>(src);
      CHECK(GetMaxspeedConverter().IsValidMacro(forward), (i));
      CHECK(GetMaxspeedConverter().IsValidMacro(backward), (i));

      auto const forwardSpeed = GetMaxspeedConverter().MacroToSpeed(static_cast<SpeedMacro>(forward));
      auto const backwardSpeed = GetMaxspeedConverter().MacroToSpeed(static_cast<SpeedMacro>(backward));
      CHECK(forwardSpeed.IsValid(), (i));
      CHECK(backwardSpeed.IsValid(), (i));
      CHECK(HaveSameUnits(forwardSpeed, backwardSpeed), (i));

      measurement_utils::Units units = measurement_utils::Units::Metric;
      if (forwardSpeed.IsNumeric())
        units = forwardSpeed.m_units;
      else if (backwardSpeed.IsNumeric())
        units = backwardSpeed.m_units;
      // Note. If neither |forwardSpeed| nor |backwardSpeed| are numeric it means
      // both of them have value "walk" or "none". So the units are not relevant for this case.

      maxspeeds.m_bidirectionalMaxspeeds.emplace_back(fid, units, forwardSpeed.m_speed,
                                                      backwardSpeed.m_speed);
    }
  }

private:
  static uint16_t constexpr kLastVersion = 0;

  struct Header
  {
  public:
    template <class Sink>
    void Serialize(Sink & sink) const
    {
      WriteToSink(sink, m_version);
      WriteToSink(sink, m_endianness);
      WriteToSink(sink, m_forwardMaxspeedOffset);
      WriteToSink(sink, m_bidirectionalMaxspeedOffset);
      WriteToSink(sink, m_bidirectionalMaxspeedNumber);
    }

    template <class Source>
    void Deserialize(Source & src)
    {
      m_version = ReadPrimitiveFromSource<decltype(m_version)>(src);
      m_endianness = ReadPrimitiveFromSource<decltype(m_endianness)>(src);
      m_forwardMaxspeedOffset = ReadPrimitiveFromSource<decltype(m_forwardMaxspeedOffset)>(src);
      m_bidirectionalMaxspeedOffset = ReadPrimitiveFromSource<decltype(m_bidirectionalMaxspeedOffset)>(src);
      m_bidirectionalMaxspeedNumber = ReadPrimitiveFromSource<decltype(m_bidirectionalMaxspeedNumber)>(src);
    }

    uint16_t m_version = kLastVersion;
    // Field |m_endianness| is reserved for endianness of the section.
    uint16_t m_endianness = 0;
    uint32_t m_forwardMaxspeedOffset = 0;
    uint32_t m_bidirectionalMaxspeedOffset = 0;
    uint32_t m_bidirectionalMaxspeedNumber = 0;
  };

  static_assert(sizeof(Header) == 16, "Wrong header size of maxspeed section.");
};
}  // namespace routing
