#pragma once

#include "routing/coding.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/road_access.hpp"
#include "routing/segment.hpp"
#include "routing/vehicle_mask.hpp"

#include "coding/bit_streams.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <vector>

namespace routing
{
class RoadAccessSerializer final
{
public:
  using RoadAccessTypesMap = std::map<Segment, RoadAccess::Type>;
  using RoadAccessByVehicleType = std::array<RoadAccess, static_cast<size_t>(VehicleType::Count)>;

  RoadAccessSerializer() = delete;

  template <class Sink>
  static void Serialize(Sink & sink, RoadAccessByVehicleType const & roadAccessByType)
  {
    uint32_t const header = kLatestVersion;
    WriteToSink(sink, header);

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
      SerializeOneVehicleType(sink, roadAccessByType[i].GetSegmentTypes());
      sectionSizes[i] = base::checked_cast<uint32_t>(sink.Pos() - pos);
    }

    auto const endPos = sink.Pos();
    sink.Seek(sectionSizesPos);
    for (size_t i = 0; i < sectionSizes.size(); ++i)
      WriteToSink(sink, sectionSizes[i]);
    sink.Seek(endPos);
  }

  template <class Source>
  static void Deserialize(Source & src, VehicleType vehicleType, RoadAccess & roadAccess)
  {
    auto const subsectionNumberToVehicleType = [](uint32_t version, size_t subsection)
    {
      if (version == 0)
      {
        switch (subsection)
        {
        case 0: return VehicleType::Pedestrian;
        case 1: return VehicleType::Bicycle;
        case 2: return VehicleType::Car;
        default: return VehicleType::Count;
        }
      }
      return static_cast<VehicleType>(subsection);
    };

    uint32_t const header = ReadPrimitiveFromSource<uint32_t>(src);

    std::vector<uint32_t> sectionSizes;
    for (size_t i = 0; subsectionNumberToVehicleType(header, i) < VehicleType::Count; ++i)
      sectionSizes.push_back(ReadPrimitiveFromSource<uint32_t>(src));

    for (size_t i = 0; subsectionNumberToVehicleType(header, i) < VehicleType::Count; ++i)
    {
      if (vehicleType != subsectionNumberToVehicleType(header, i))
      {
        src.Skip(sectionSizes[i]);
        continue;
      }

      RoadAccessTypesMap m;
      DeserializeOneVehicleType(src, m);

      roadAccess.SetSegmentTypes(std::move(m));
    }
  }

private:
  template <typename Sink>
  static void SerializeOneVehicleType(Sink & sink, RoadAccessTypesMap const & m)
  {
    std::array<std::vector<Segment>, static_cast<size_t>(RoadAccess::Type::Count)>
        segmentsByRoadAccessType;
    for (auto const & kv : m)
      segmentsByRoadAccessType[static_cast<size_t>(kv.second)].push_back(kv.first);

    for (auto & segs : segmentsByRoadAccessType)
    {
      std::sort(segs.begin(), segs.end());
      SerializeSegments(sink, segs);
    }
  }

  template <typename Source>
  static void DeserializeOneVehicleType(Source & src, RoadAccessTypesMap & m)
  {
    m.clear();
    for (size_t i = 0; i < static_cast<size_t>(RoadAccess::Type::Count); ++i)
    {
      std::vector<Segment> segs;
      DeserializeSegments(src, segs);
      for (auto const & seg : segs)
        m[seg] = static_cast<RoadAccess::Type>(i);
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

  uint32_t static const kLatestVersion;
};
}  // namespace routing
