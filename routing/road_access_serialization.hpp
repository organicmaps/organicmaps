#pragma once

#include "routing/num_mwm_id.hpp"
#include "routing/road_access.hpp"
#include "routing/segment.hpp"

#include "coding/bit_streams.hpp"
#include "coding/elias_coder.hpp"
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

  RoadAccessSerializer() = delete;

  template <class Sink>
  static void Serialize(Sink & sink, RoadAccess const & roadAccess)
  {
    uint32_t const header = kLatestVersion;
    WriteToSink(sink, header);

    for (auto const routerType : RoadAccess::GetSupportedRouterTypes())
      SerializeOneRouterType(sink, roadAccess.GetTypes(routerType));
  }

  template <class Source>
  static void Deserialize(Source & src, RoadAccess & roadAccess)
  {
    uint32_t const header = ReadPrimitiveFromSource<uint32_t>(src);
    CHECK_EQUAL(header, kLatestVersion, ());

    for (auto const routerType : RoadAccess::GetSupportedRouterTypes())
    {
      RoadAccessTypesMap m;
      DeserializeOneRouterType(src, m);
      roadAccess.SetTypes(routerType, std::move(m));
    }
  }

private:
  template <typename Sink>
  static void SerializeOneRouterType(Sink & sink, RoadAccessTypesMap const & m)
  {
    std::array<std::vector<Segment>, static_cast<size_t>(RoadAccess::Type::Count)>
        segmentsByRoadAccessType;
    for (auto const & kv : m)
      segmentsByRoadAccessType[static_cast<size_t>(kv.second)].push_back(kv.first);

    for (auto const & segs : segmentsByRoadAccessType)
      SerializeSegments(sink, segs);
  }

  template <typename Source>
  static void DeserializeOneRouterType(Source & src, RoadAccessTypesMap & m)
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
  static void SerializeSegments(Sink & sink, vector<Segment> const & segments)
  {
    vector<uint32_t> featureIds(segments.size());
    vector<uint32_t> segmentIndices(segments.size());
    vector<bool> isForward(segments.size());

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
      for (auto const & fid : featureIds)
      {
        uint64_t const fidDiff = static_cast<uint64_t>(fid - prevFid);
        bool ok = coding::GammaCoder::Encode(bitWriter, fidDiff + 1);
        ASSERT(ok, ());
        UNUSED_VALUE(ok);
        prevFid = fid;
      }

      for (auto const & s : segmentIndices)
      {
        bool ok = coding::GammaCoder::Encode(bitWriter, s + 1);
        ASSERT(ok, ());
        UNUSED_VALUE(ok);
      }

      for (auto const & val : isForward)
        bitWriter.Write(val ? 1 : 0, 1 /* numBits */);
    }
  }

  template <typename Source>
  static void DeserializeSegments(Source & src, vector<Segment> & segments)
  {
    auto const n = static_cast<size_t>(ReadVarUint<uint64_t>(src));

    vector<uint32_t> featureIds(n);
    vector<size_t> segmentIndices(n);
    vector<bool> isForward(n);

    {
      BitReader<decltype(src)> bitReader(src);
      uint32_t prevFid = 0;
      for (size_t i = 0; i < n; ++i)
      {
        prevFid += coding::GammaCoder::Decode(bitReader) - 1;
        featureIds[i] = prevFid;
      }

      for (size_t i = 0; i < n; ++i)
        segmentIndices[i] = coding::GammaCoder::Decode(bitReader) - 1;

      for (size_t i = 0; i < n; ++i)
        isForward[i] = bitReader.Read(1) > 0;

      // Read the padding bits.
      auto bitsRead = bitReader.BitsRead();
      while (bitsRead % CHAR_BIT != 0)
      {
        bitReader.Read(1);
        ++bitsRead;
      }
    }

    segments.clear();
    segments.reserve(n);
    for (size_t i = 0; i < n; ++i)
      segments.emplace_back(kFakeNumMwmId, featureIds[i], segmentIndices[i], isForward[i]);
  }

  uint32_t static const kLatestVersion;
};
}  // namespace routing
