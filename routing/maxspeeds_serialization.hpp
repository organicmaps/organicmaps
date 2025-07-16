#pragma once

#include "routing/maxspeeds.hpp"

#include "routing_common/vehicle_model.hpp"

#include "coding/reader.hpp"
#include "coding/succinct_mapper.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace routing
{
/// \brief
/// Section name: MAXSPEEDS_FILE_TAG.
/// Description: keeping value of maxspeed tag for roads.
/// Section tables:
/// * Header
/// * HighwayType -> Average speed macro
/// * Forward speeds:
///   elias_fano with feature ids which have maxspeed tag in forward direction only
///   SimpleDenseCoding with speed macros
/// * Bidirectional speeds in raw table:
///   <varint(delta(Feature id))><Forward speed macro><Backward speed macro>
///
/// \note elias_fano is better than raw table in case of high density of entries, so we assume that
///  Bidirectional speeds count *is much less than* Forward speeds count.
class MaxspeedsSerializer
{
  // 0 - start version
  // 1 - added HighwayType -> SpeedMacro
  // 2 - HighwayType -> SpeedMacro for inside/outside a city
  using VersionT = uint16_t;
  static VersionT constexpr kVersion = 2;

public:
  MaxspeedsSerializer() = delete;

  struct FeatureSpeedMacro
  {
    FeatureSpeedMacro(uint32_t featureID, SpeedMacro forward, SpeedMacro backward = SpeedMacro::Undefined)
      : m_featureID(featureID)
      , m_forward(forward)
      , m_backward(backward)
    {
      // We store bidirectional speeds only if they are not equal,
      // see Maxspeed::IsBidirectional() comments.
      if (m_forward == m_backward)
        m_backward = SpeedMacro::Undefined;
    }

    bool IsBidirectional() const { return m_backward != SpeedMacro::Undefined; }

    uint32_t m_featureID;
    SpeedMacro m_forward;
    SpeedMacro m_backward;
  };

  using HW2SpeedMap = std::map<HighwayType, SpeedMacro>;

  static int constexpr DEFAULT_SPEEDS_COUNT = Maxspeeds::DEFAULT_SPEEDS_COUNT;

  template <class Sink>
  static void Serialize(std::vector<FeatureSpeedMacro> const & featureSpeeds, HW2SpeedMap typeSpeeds[], Sink & sink)
  {
    CHECK(base::IsSortedAndUnique(featureSpeeds.cbegin(), featureSpeeds.cend(),
                                  [](FeatureSpeedMacro const & l, FeatureSpeedMacro const & r)
    { return l.m_featureID < r.m_featureID; }),
          ());

    // Version
    auto const startOffset = sink.Pos();
    WriteToSink(sink, kVersion);

    // Header
    auto const headerOffset = sink.Pos();
    Header header;
    header.Serialize(sink);

    auto const forwardMaxspeedTableOffset = sink.Pos();

    // Saving forward (one direction) maxspeeds.
    uint32_t maxFeatureID;
    std::vector<uint8_t> forwardMaxspeeds = GetForwardMaxspeeds(featureSpeeds, maxFeatureID);

    size_t forwardCount = forwardMaxspeeds.size();
    if (forwardCount > 0)
    {
      succinct::elias_fano::elias_fano_builder builder(maxFeatureID + 1, forwardCount);
      for (auto const & e : featureSpeeds)
        if (!e.IsBidirectional())
          builder.push_back(e.m_featureID);

      coding::FreezeVisitor<Writer> visitor(sink);
      succinct::elias_fano(&builder).map(visitor);

      header.m_forwardMaxspeedOffset = static_cast<uint32_t>(sink.Pos() - forwardMaxspeedTableOffset);

      coding::SimpleDenseCoding simpleDenseCoding(forwardMaxspeeds);
      coding::Freeze(simpleDenseCoding, sink, "ForwardMaxspeeds");
    }
    else
    {
      header.m_forwardMaxspeedOffset = 0;
    }
    header.m_bidirectionalMaxspeedOffset = static_cast<uint32_t>(sink.Pos() - forwardMaxspeedTableOffset);

    // Saving bidirectional maxspeeds.
    uint32_t prevFeatureId = 0;
    for (auto const & s : featureSpeeds)
    {
      if (!s.IsBidirectional())
        continue;

      ++header.m_bidirectionalMaxspeedNumber;

      // Valid, because we did base::IsSortedAndUnique in the beginning.
      WriteVarUint(sink, s.m_featureID - prevFeatureId);
      prevFeatureId = s.m_featureID;

      WriteToSink(sink, static_cast<uint8_t>(s.m_forward));
      WriteToSink(sink, static_cast<uint8_t>(s.m_backward));
    }

    // Save HighwayType speeds, sorted by type.
    for (int ind = 0; ind < DEFAULT_SPEEDS_COUNT; ++ind)
    {
      WriteVarUint(sink, static_cast<uint32_t>(typeSpeeds[ind].size()));
      for (auto const & [type, speed] : typeSpeeds[ind])
      {
        WriteVarUint(sink, static_cast<uint32_t>(type));
        WriteToSink(sink, static_cast<uint8_t>(speed));
      }
    }

    // Finalize Header data.
    auto const endOffset = sink.Pos();
    sink.Seek(headerOffset);
    header.Serialize(sink);
    sink.Seek(endOffset);

    // Print statistics.
    LOG(LINFO, ("Serialized", forwardCount, "forward maxspeeds and", header.m_bidirectionalMaxspeedNumber,
                "bidirectional maxspeeds. Section size:", endOffset - startOffset, "bytes."));
    LOG(LINFO,
        ("Succinct EF compression ratio:", forwardCount * (4 + 1) / double(header.m_bidirectionalMaxspeedOffset)));
  }

  template <class Source>
  static void Deserialize(Source & src, Maxspeeds & maxspeeds)
  {
    CHECK(maxspeeds.IsEmpty(), ());

    // Version
    auto const version = ReadPrimitiveFromSource<VersionT>(src);

    // Header
    Header header;
    header.Deserialize(src);

    // Loading forward only speeds.
    if (header.m_forwardMaxspeedOffset != header.m_bidirectionalMaxspeedOffset)
    {
      CHECK_LESS(header.m_forwardMaxspeedOffset, header.m_bidirectionalMaxspeedOffset, ());

      // Reading maxspeed information for features which have only forward maxspeed.
      std::vector<uint8_t> forwardTableData(header.m_forwardMaxspeedOffset);
      src.Read(forwardTableData.data(), forwardTableData.size());
      maxspeeds.m_forwardMaxspeedTableRegion = std::make_unique<CopiedMemoryRegion>(std::move(forwardTableData));
      coding::Map(maxspeeds.m_forwardMaxspeedsTable, maxspeeds.m_forwardMaxspeedTableRegion->ImmutableData(),
                  "ForwardMaxspeedsTable");

      std::vector<uint8_t> forwardData(header.m_bidirectionalMaxspeedOffset - header.m_forwardMaxspeedOffset);
      src.Read(forwardData.data(), forwardData.size());
      maxspeeds.m_forwardMaxspeedRegion = std::make_unique<CopiedMemoryRegion>(std::move(forwardData));
      Map(maxspeeds.m_forwardMaxspeeds, maxspeeds.m_forwardMaxspeedRegion->ImmutableData(), "ForwardMaxspeeds");
    }

    // Loading bidirectional speeds.
    auto const & converter = GetMaxspeedConverter();
    auto ReadSpeed = [&](size_t i)
    {
      uint8_t const idx = ReadPrimitiveFromSource<uint8_t>(src);
      auto const speed = converter.MacroToSpeed(static_cast<SpeedMacro>(idx));
      CHECK(speed.IsValid(), (i));
      return speed;
    };

    maxspeeds.m_bidirectionalMaxspeeds.reserve(header.m_bidirectionalMaxspeedNumber);
    uint32_t featureId = 0;
    for (size_t i = 0; i < header.m_bidirectionalMaxspeedNumber; ++i)
    {
      auto const delta = ReadVarUint<uint32_t>(src);
      auto const forwardSpeed = ReadSpeed(i);
      auto const backwardSpeed = ReadSpeed(i);

      CHECK(HaveSameUnits(forwardSpeed, backwardSpeed), (i));

      // Note. If neither |forwardSpeed| nor |backwardSpeed| are numeric, it means
      // both of them have value "walk" or "none". So the units are not relevant for this case.
      measurement_utils::Units units = measurement_utils::Units::Metric;
      if (forwardSpeed.IsNumeric())
        units = forwardSpeed.GetUnits();
      else if (backwardSpeed.IsNumeric())
        units = backwardSpeed.GetUnits();

      featureId += delta;
      maxspeeds.m_bidirectionalMaxspeeds.emplace_back(featureId, units, forwardSpeed.GetSpeed(),
                                                      backwardSpeed.GetSpeed());
    }

    // Load HighwayType speeds.
    if (version >= 2)
    {
      for (int ind = 0; ind < DEFAULT_SPEEDS_COUNT; ++ind)
      {
        uint32_t const count = ReadVarUint<uint32_t>(src);
        for (uint32_t i = 0; i < count; ++i)
        {
          auto const type = static_cast<HighwayType>(ReadVarUint<uint32_t>(src));
          auto const speed = converter.MacroToSpeed(static_cast<SpeedMacro>(ReadPrimitiveFromSource<uint8_t>(src)));
          // Constraint should be met, but unit tests use different input Units. No problem here.
          // ASSERT_EQUAL(speed.GetUnits(), measurement_utils::Units::Metric, ());
          maxspeeds.m_defaultSpeeds[ind][type] = speed.GetSpeed();
        }
      }
    }
  }

private:
  static std::vector<uint8_t> GetForwardMaxspeeds(std::vector<FeatureSpeedMacro> const & speeds,
                                                  uint32_t & maxFeatureID);

  struct Header
  {
  public:
    template <class Sink>
    void Serialize(Sink & sink) const
    {
      WriteToSink(sink, m_endianness);
      WriteToSink(sink, m_forwardMaxspeedOffset);
      WriteToSink(sink, m_bidirectionalMaxspeedOffset);
      WriteToSink(sink, m_bidirectionalMaxspeedNumber);
    }

    template <class Source>
    void Deserialize(Source & src)
    {
      m_endianness = ReadPrimitiveFromSource<decltype(m_endianness)>(src);
      m_forwardMaxspeedOffset = ReadPrimitiveFromSource<decltype(m_forwardMaxspeedOffset)>(src);
      m_bidirectionalMaxspeedOffset = ReadPrimitiveFromSource<decltype(m_bidirectionalMaxspeedOffset)>(src);
      m_bidirectionalMaxspeedNumber = ReadPrimitiveFromSource<decltype(m_bidirectionalMaxspeedNumber)>(src);
    }

    // Field |m_endianness| is reserved for endianness of the section.
    uint16_t m_endianness = 0;
    uint32_t m_forwardMaxspeedOffset = 0;
    uint32_t m_bidirectionalMaxspeedOffset = 0;
    uint32_t m_bidirectionalMaxspeedNumber = 0;
  };
};
}  // namespace routing
