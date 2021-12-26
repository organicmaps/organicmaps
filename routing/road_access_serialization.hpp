#pragma once

#include "routing/coding.hpp"
#include "routing/opening_hours_serdes.hpp"
#include "routing/road_access.hpp"
#include "routing/road_point.hpp"
#include "routing/segment.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "platform/platform.hpp"

#include "coding/bit_streams.hpp"
#include "coding/reader.hpp"
#include "coding/sha1.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "3party/skarupke/flat_hash_map.hpp"

namespace routing
{
class RoadAccessSerializer final
{
public:
  using WayToAccess = RoadAccess::WayToAccess;
  using PointToAccess = RoadAccess::PointToAccess;
  using WayToAccessConditional = RoadAccess::WayToAccessConditional;
  using PointToAccessConditional = RoadAccess::PointToAccessConditional;
  using RoadAccessByVehicleType = std::array<RoadAccess, static_cast<size_t>(VehicleType::Count)>;

  enum class Header
  {
    TheFirstVersionRoadAccess = 0, // Version of section roadaccess in 2017.
    WithoutAccessConditional = 1,  // Section roadaccess before conditional was implemented.
    WithAccessConditional = 2      // Section roadaccess with conditional.
  };

  RoadAccessSerializer() = delete;

  template <class Sink>
  static void Serialize(Sink & sink, RoadAccessByVehicleType const & roadAccessByType)
  {
    Header const header = kLatestVersion;
    WriteToSink(sink, header);
    SerializeAccess(sink, roadAccessByType);
    SerializeAccessConditional(sink, roadAccessByType);
  }

  template <class Source>
  static void Deserialize(Source & src, VehicleType vehicleType, RoadAccess & roadAccess,
                          std::string const & mwmPath)
  {
    auto const readHeader = ReadPrimitiveFromSource<uint32_t>(src);
    auto const header = static_cast<Header>(readHeader);
    CHECK_LESS_OR_EQUAL(header, kLatestVersion, ());
    switch (header)
    {
    case Header::TheFirstVersionRoadAccess:
      break; // Version of 2017. Unsupported.
    case Header::WithoutAccessConditional:
    {
      DeserializeAccess(src, vehicleType, roadAccess);
      break;
    }
    case Header::WithAccessConditional:
    {
      DeserializeAccess(src, vehicleType, roadAccess);
      // access:conditional should be switch off for release 10.0 and probably for the next one.
      // It means that they should be switch off for cross_mwm section generation and for runtime.
      // To switch on access:conditional the line below should be uncommented.
      // Also tests in routing/routing_tests/road_access_test.cpp should be uncommented.
      // DeserializeAccessConditional(src, vehicleType, roadAccess);
      break;
    }
    default:
    {
      LOG(LERROR, ("Wrong RoadAccessSerializer header in", mwmPath));
      UNREACHABLE();
    }
    }
  }

private:
  inline static Header const kLatestVersion = Header::WithAccessConditional;

  class AccessPosition
  {
  public:
    static AccessPosition MakeWayAccess(uint32_t featureId)
    {
      return {featureId, 0 /* wildcard pointId for way access */};
    }

    static AccessPosition MakePointAccess(uint32_t featureId, uint32_t pointId)
    {
      return {featureId, pointId + 1};
    }

    AccessPosition() = default;
    AccessPosition(uint32_t featureId, uint32_t pointId)
      : m_featureId(featureId), m_pointId(pointId)
    {
    }

    bool operator<(AccessPosition const & rhs) const
    {
      return std::tie(m_featureId, m_pointId) < std::tie(rhs.m_featureId, rhs.m_pointId);
    }

    uint32_t GetFeatureId() const { return m_featureId; }
    uint32_t GetPointId() const
    {
      ASSERT(IsNode(), ());
      return m_pointId - 1;
    }

    bool IsWay() const { return m_pointId == 0; }
    bool IsNode() const { return m_pointId != 0; }

  private:
    uint32_t m_featureId = 0;
    uint32_t m_pointId = 0;
  };

  template <class Sink>
  static void SerializeAccess(Sink & sink, RoadAccessByVehicleType const & roadAccessByType)
  {
    auto const sectionSizesPos = sink.Pos();
    std::array<uint32_t, static_cast<size_t>(VehicleType::Count)> sectionSizes;
    for (size_t i = 0; i < sectionSizes.size(); ++i)
    {
      sectionSizes[i] = 0;
      WriteToSink(sink, sectionSizes[i]);
    }

    for (size_t i = 0; i < static_cast<size_t>(VehicleType::Count); ++i)
    {
      auto const pos = sink.Pos();
      SerializeOneVehicleType(sink, roadAccessByType[i].GetWayToAccess(),
                              roadAccessByType[i].GetPointToAccess());
      sectionSizes[i] = base::checked_cast<uint32_t>(sink.Pos() - pos);
    }

    auto const endPos = sink.Pos();
    sink.Seek(sectionSizesPos);
    for (auto const sectionSize : sectionSizes)
      WriteToSink(sink, sectionSize);

    sink.Seek(endPos);
  }

  template <class Source>
  static void DeserializeAccess(Source & src, VehicleType vehicleType, RoadAccess & roadAccess)
  {
    std::array<uint32_t, static_cast<size_t>(VehicleType::Count)> sectionSizes{};
    static_assert(static_cast<int>(VehicleType::Count) == 4,
                  "It is assumed below that there are only 4 vehicle types and we store 4 numbers "
                  "of sections size. If you add or remove vehicle type you should up "
                  "|kLatestVersion| and save back compatibility here.");

    for (auto & sectionSize : sectionSizes)
      sectionSize = ReadPrimitiveFromSource<uint32_t>(src);

    for (size_t i = 0; i < sectionSizes.size(); ++i)
    {
      auto const sectionVehicleType = static_cast<VehicleType>(i);
      if (sectionVehicleType != vehicleType)
      {
        src.Skip(sectionSizes[i]);
        continue;
      }

      WayToAccess wayToAccess;
      PointToAccess pointToAccess;
      DeserializeOneVehicleType(src, wayToAccess, pointToAccess);

      roadAccess.SetAccess(std::move(wayToAccess), std::move(pointToAccess));
    }
  }

  template <class Sink>
  static void SerializeAccessConditional(Sink & sink,
                                         RoadAccessByVehicleType const & roadAccessByType)
  {
    auto const sectionSizesPos = sink.Pos();
    std::array<uint32_t, static_cast<size_t>(VehicleType::Count)> sectionSizes;
    for (size_t i = 0; i < sectionSizes.size(); ++i)
    {
      sectionSizes[i] = 0;
      WriteToSink(sink, sectionSizes[i]);
    }

    for (size_t i = 0; i < static_cast<size_t>(VehicleType::Count); ++i)
    {
      auto const pos = sink.Pos();
      SerializeConditionalOneVehicleType(sink, roadAccessByType[i].GetWayToAccessConditional(),
                                         roadAccessByType[i].GetPointToAccessConditional());
      sectionSizes[i] = base::checked_cast<uint32_t>(sink.Pos() - pos);
    }

    auto const endPos = sink.Pos();
    sink.Seek(sectionSizesPos);
    for (auto const sectionSize : sectionSizes)
      WriteToSink(sink, sectionSize);

    sink.Seek(endPos);
  }

  template <class Source>
  static void DeserializeAccessConditional(Source & src, VehicleType vehicleType,
                                           RoadAccess & roadAccess)
  {
    std::array<uint32_t, static_cast<size_t>(VehicleType::Count)> sectionSizes{};
    static_assert(static_cast<int>(VehicleType::Count) == 4,
                  "It is assumed below that there are only 4 vehicle types and we store 4 numbers "
                  "of sections size. If you add or remove vehicle type you should up "
                  "|kLatestVersion| and save back compatibility here.");

    for (auto & sectionSize : sectionSizes)
      sectionSize = ReadPrimitiveFromSource<uint32_t>(src);

    for (size_t i = 0; i < sectionSizes.size(); ++i)
    {
      auto const sectionVehicleType = static_cast<VehicleType>(i);
      if (sectionVehicleType != vehicleType)
      {
        src.Skip(sectionSizes[i]);
        continue;
      }

      WayToAccessConditional wayToAccessConditional;
      PointToAccessConditional pointToAccessConditional;
      DeserializeConditionalOneVehicleType(src, wayToAccessConditional, pointToAccessConditional);

      roadAccess.SetAccessConditional(std::move(wayToAccessConditional),
                                      std::move(pointToAccessConditional));
    }
  }

  template <typename Sink>
  static void SerializeOneVehicleType(Sink & sink, WayToAccess const & wayToAccess,
                                      PointToAccess const & pointToAccess)
  {
    std::array<std::vector<Segment>, static_cast<size_t>(RoadAccess::Type::Count)>
        segmentsByRoadAccessType;
    for (auto const & kv : wayToAccess)
    {
      segmentsByRoadAccessType[static_cast<size_t>(kv.second)].push_back(
          Segment(kFakeNumMwmId, kv.first, 0 /* wildcard segmentIdx */, true /* widcard forward */));
    }
    // For nodes we store |pointId + 1| because 0 is reserved for wildcard segmentIdx.
    for (auto const & kv : pointToAccess)
    {
      segmentsByRoadAccessType[static_cast<size_t>(kv.second)].push_back(
          Segment(kFakeNumMwmId, kv.first.GetFeatureId(), kv.first.GetPointId() + 1, true));
    }

    for (auto & segs : segmentsByRoadAccessType)
    {
      std::sort(segs.begin(), segs.end());
      SerializeSegments(sink, segs);
    }
  }

  template <typename Source>
  static void DeserializeOneVehicleType(Source & src, WayToAccess & wayToAccess,
                                        PointToAccess & pointToAccess)
  {
    wayToAccess.clear();
    pointToAccess.clear();
    for (size_t i = 0; i < static_cast<size_t>(RoadAccess::Type::Count); ++i)
    {
      // An earlier version allowed blocking any segment of a feature (or the entire feature
      // by providing a wildcard segment index).
      // At present, we either block the feature entirely or block any of its road points. The
      // the serialization code remains the same, although its semantics changes as we now
      // work with point indices instead of segment indices.
      std::vector<Segment> segs;
      DeserializeSegments(src, segs);
      for (auto const & seg : segs)
      {
        if (seg.GetSegmentIdx() == 0)
        {
          // Wildcard segmentIdx.
          wayToAccess[seg.GetFeatureId()] = static_cast<RoadAccess::Type>(i);
        }
        else
        {
          // For nodes we store |pointId + 1| because 0 is reserved for wildcard segmentIdx.
          pointToAccess[RoadPoint(seg.GetFeatureId(), seg.GetSegmentIdx() - 1)] =
              static_cast<RoadAccess::Type>(i);
        }
      }
    }
  }

  using PositionToAccessConditional = std::pair<AccessPosition, RoadAccess::Conditional::Access>;

  template <typename Sink>
  static void SerializeConditionalOneVehicleType(
      Sink & sink, WayToAccessConditional const & wayToAccessConditional,
      PointToAccessConditional const & pointToAccessConditional)
  {
    auto constexpr kAccessTypesCount = static_cast<size_t>(RoadAccess::Type::Count);
    std::array<std::vector<PositionToAccessConditional>, kAccessTypesCount> positionsByAccessType;

    for (auto const & [featureId, conditional] : wayToAccessConditional)
    {
      auto const position = AccessPosition::MakeWayAccess(featureId);
      for (auto const & access : conditional.GetAccesses())
        positionsByAccessType[static_cast<size_t>(access.m_type)].emplace_back(position, access);
    }

    // For nodes we store |pointId + 1| because 0 is reserved for wildcard segmentIdx.
    for (auto const & [roadPoint, conditional] : pointToAccessConditional)
    {
      auto const position =
          AccessPosition::MakePointAccess(roadPoint.GetFeatureId(), roadPoint.GetPointId());

      for (auto const & access : conditional.GetAccesses())
        positionsByAccessType[static_cast<size_t>(access.m_type)].emplace_back(position, access);
    }

    for (auto & positionsConditional : positionsByAccessType)
    {
      std::sort(positionsConditional.begin(), positionsConditional.end(),
                [](auto const & lhs, auto const & rhs) {
                  auto const & lhsAccessPosition = lhs.first;
                  auto const & rhsAccessPosition = rhs.first;
                  return lhsAccessPosition < rhsAccessPosition;
                });

      SerializePositionsAccessConditional(sink, positionsConditional);
    }
  }

  template <typename Source>
  static void DeserializeConditionalOneVehicleType(
      Source & src, WayToAccessConditional & wayToAccessConditional,
      PointToAccessConditional & pointToAccessConditional)
  {
    wayToAccessConditional.clear();
    pointToAccessConditional.clear();

    for (size_t i = 0; i < static_cast<size_t>(RoadAccess::Type::Count); ++i)
    {
      std::vector<PositionToAccessConditional> positions;

      auto const accessType = static_cast<RoadAccess::Type>(i);
      DeserializePositionsAccessConditional(src, positions);
      for (auto & [position, access] : positions)
      {
        if (position.IsWay())
        {
          auto const featureId = position.GetFeatureId();
          wayToAccessConditional[featureId].Insert(accessType, std::move(access.m_openingHours));
        }
        else if (position.IsNode())
        {
          RoadPoint const roadPoint(position.GetFeatureId(), position.GetPointId());
          pointToAccessConditional[roadPoint].Insert(accessType, std::move(access.m_openingHours));
        }
        else
        {
          UNREACHABLE();
        }
      }
    }
  }

  // todo(@m) This code borrows heavily from traffic/traffic_info.hpp:SerializeTrafficKeys.
  template <typename Sink>
  static void SerializeSegments(Sink & sink, std::vector<Segment> const & segments)
  {
    std::vector<uint32_t> featureIds(segments.size());
    std::vector<uint32_t> segmentIndices(segments.size());
    std::vector<bool> isForward(segments.size());

    for (size_t i = 0; i < segments.size(); ++i)
    {
      auto const & seg = segments[i];
      CHECK_EQUAL(seg.GetMwmId(), kFakeNumMwmId,
                  ("Numeric mwm ids are temporary and must not be serialized."));
      featureIds[i] = seg.GetFeatureId();
      segmentIndices[i] = seg.GetSegmentIdx();
      isForward[i] = seg.IsForward();
    }

    WriteVarUint(sink, segments.size());

    {
      BitWriter<Sink> bitWriter(sink);

      uint32_t prevFid = 0;
      for (auto const fid : featureIds)
      {
        CHECK_GREATER_OR_EQUAL(fid, prevFid, ());
        uint64_t const fidDiff = static_cast<uint64_t>(fid - prevFid);
        WriteGamma(bitWriter, fidDiff + 1);
        prevFid = fid;
      }

      for (auto const idx : segmentIndices)
        WriteGamma(bitWriter, idx + 1);

      for (auto const val : isForward)
        bitWriter.Write(val ? 1 : 0, 1 /* numBits */);
    }
  }

  template <typename Source>
  static void DeserializeSegments(Source & src, std::vector<Segment> & segments)
  {
    auto const n = static_cast<size_t>(ReadVarUint<uint64_t>(src));

    std::vector<uint32_t> featureIds(n);
    std::vector<uint32_t> segmentIndices(n);
    std::vector<bool> isForward(n);

    BitReader<Source> bitReader(src);
    uint32_t prevFid = 0;
    for (size_t i = 0; i < n; ++i)
    {
      prevFid += ReadGamma<uint64_t>(bitReader) - 1;
      featureIds[i] = prevFid;
    }

    for (size_t i = 0; i < n; ++i)
      segmentIndices[i] = ReadGamma<uint32_t>(bitReader) - 1;
    for (size_t i = 0; i < n; ++i)
      isForward[i] = bitReader.Read(1) > 0;

    // Read the padding bits.
    auto bitsRead = bitReader.BitsRead();
    while (bitsRead % CHAR_BIT != 0)
    {
      bitReader.Read(1);
      ++bitsRead;
    }

    segments.clear();
    segments.reserve(n);
    for (size_t i = 0; i < n; ++i)
      segments.emplace_back(kFakeNumMwmId, featureIds[i], segmentIndices[i], isForward[i]);
  }

  template <typename Sink>
  static void SerializePositionsAccessConditional(
      Sink & sink, std::vector<PositionToAccessConditional> const & positionsAccessConditional)
  {
    auto const sizePos = sink.Pos();
    auto openingHoursSerializer = GetOpeningHoursSerDesForRouting();

    size_t successWritten = 0;
    WriteToSink(sink, successWritten);

    {
      BitWriter<Sink> bitWriter(sink);
      uint32_t prevFeatureId = 0;
      for (auto const & [position, accessConditional] : positionsAccessConditional)
      {
        if (!openingHoursSerializer.Serialize(bitWriter, accessConditional.m_openingHours))
          continue;

        uint32_t const currentFeatureId = position.GetFeatureId();
        CHECK_GREATER_OR_EQUAL(currentFeatureId, prevFeatureId, ());
        uint32_t const featureIdDiff = currentFeatureId - prevFeatureId;
        prevFeatureId = currentFeatureId;

        WriteGamma(bitWriter, featureIdDiff + 1);
        auto const pointId = position.IsWay() ? 0 : position.GetPointId() + 1;
        WriteGamma(bitWriter, pointId + 1);
        ++successWritten;
      }
    }

    auto const endPos = sink.Pos();
    sink.Seek(sizePos);
    WriteToSink(sink, successWritten);
    sink.Seek(endPos);
  }

  template <typename Source>
  static void DeserializePositionsAccessConditional(
      Source & src, std::vector<PositionToAccessConditional> & positionsAccessConditional)
  {
    positionsAccessConditional.clear();

    auto openingHoursDeserializer = GetOpeningHoursSerDesForRouting();
    auto const size = ReadPrimitiveFromSource<size_t>(src);

    positionsAccessConditional.reserve(size);
    uint32_t prevFeatureId = 0;
    BitReader<Source> bitReader(src);
    for (size_t i = 0; i < size; ++i)
    {
      auto oh = openingHoursDeserializer.Deserialize(bitReader);
      auto const featureIdDiff = ReadGamma<uint32_t>(bitReader) - 1;
      auto const featureId = prevFeatureId + featureIdDiff;
      prevFeatureId = featureId;
      auto const pointId = ReadGamma<uint32_t>(bitReader) - 1;

      AccessPosition const position(featureId, pointId);
      RoadAccess::Conditional::Access access(RoadAccess::Type::Count, std::move(oh));
      positionsAccessConditional.emplace_back(position, std::move(access));
    }
  }

  static OpeningHoursSerDes GetOpeningHoursSerDesForRouting()
  {
    OpeningHoursSerDes openingHoursSerDes;
    openingHoursSerDes.Enable(OpeningHoursSerDes::Header::Bits::Year);
    openingHoursSerDes.Enable(OpeningHoursSerDes::Header::Bits::Month);
    openingHoursSerDes.Enable(OpeningHoursSerDes::Header::Bits::MonthDay);
    openingHoursSerDes.Enable(OpeningHoursSerDes::Header::Bits::WeekDay);
    openingHoursSerDes.Enable(OpeningHoursSerDes::Header::Bits::Hours);
    openingHoursSerDes.Enable(OpeningHoursSerDes::Header::Bits::Minutes);
    return openingHoursSerDes;
  }
};

std::string DebugPrint(RoadAccessSerializer::Header const & header);
}  // namespace routing
