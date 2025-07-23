#pragma once

#include "routing/latlon_with_altitude.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "coding/point_coding.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace routing
{

using RegionSegmentId = uint32_t;

struct CrossBorderSegmentEnding
{
  CrossBorderSegmentEnding() = default;
  CrossBorderSegmentEnding(m2::PointD const & point, NumMwmId const & mwmId);
  CrossBorderSegmentEnding(ms::LatLon const & point, NumMwmId const & mwmId);

  LatLonWithAltitude m_point;
  NumMwmId m_numMwmId = std::numeric_limits<NumMwmId>::max();
};

struct CrossBorderSegment
{
  CrossBorderSegmentEnding m_start;
  CrossBorderSegmentEnding m_end;

  double m_weight = 0.0;
};

using CrossBorderSegments = std::unordered_map<RegionSegmentId, CrossBorderSegment>;
using MwmIdToSegmentIds = std::unordered_map<NumMwmId, std::vector<RegionSegmentId>>;

struct CrossBorderGraph
{
  void AddCrossBorderSegment(RegionSegmentId segId, CrossBorderSegment const & segment);

  CrossBorderSegments m_segments;
  MwmIdToSegmentIds m_mwms;
};

class CrossBorderGraphSerializer
{
public:
  CrossBorderGraphSerializer() = delete;

  template <class Sink>
  static void Serialize(CrossBorderGraph const & graph, Sink & sink, std::shared_ptr<NumMwmIds> numMwmIds);

  template <class Source>
  static void Deserialize(CrossBorderGraph & graph, Source & src, std::shared_ptr<NumMwmIds> numMwmIds);

private:
  static uint32_t constexpr kVersion = 1;
  static double constexpr kDouble2Int = 1.0E6;

  struct Header
  {
    Header() = default;
    explicit Header(CrossBorderGraph const & graph, uint32_t version = kVersion);

    template <class Sink>
    void Serialize(Sink & sink) const;

    template <class Source>
    void Deserialize(Source & src);

    uint32_t m_numRegions = 0;
    uint32_t m_numRoads = 0;

    uint32_t m_version = kVersion;
  };

  static size_t constexpr kBitsForDouble = 30;

  static uint32_t Hash(std::string const & s);
};

// static
template <class Sink>
void CrossBorderGraphSerializer::Serialize(CrossBorderGraph const & graph, Sink & sink,
                                           std::shared_ptr<NumMwmIds> numMwmIds)
{
  Header header(graph);
  header.Serialize(sink);

  std::set<uint32_t> mwmNameHashes;

  for (auto it = graph.m_mwms.begin(); it != graph.m_mwms.end(); ++it)
  {
    auto const & mwmId = it->first;
    std::string const & mwmName = numMwmIds->GetFile(mwmId).GetName();
    auto const hash = Hash(mwmName);

    // Triggering of this check during the maps build means that the mwm set has been changed and
    // current hash function Hash(mwmName) no longer suits it and should be replaced.
    CHECK(mwmNameHashes.emplace(hash).second, (mwmId, mwmName, hash));
  }

  CHECK_LESS(mwmNameHashes.size(), std::numeric_limits<NumMwmId>::max(), ());

  for (auto hash : mwmNameHashes)
    WriteToSink(sink, hash);

  auto writeSegEnding = [&](CrossBorderSegmentEnding const & ending)
  {
    auto const & coord = ending.m_point.GetLatLon();
    WriteToSink(sink, DoubleToUint32(coord.m_lat, ms::LatLon::kMinLat, ms::LatLon::kMaxLat, kBitsForDouble));
    WriteToSink(sink, DoubleToUint32(coord.m_lon, ms::LatLon::kMinLon, ms::LatLon::kMaxLon, kBitsForDouble));

    auto const & mwmNameHash = Hash(numMwmIds->GetFile(ending.m_numMwmId).GetName());
    auto it = mwmNameHashes.find(mwmNameHash);
    CHECK(it != mwmNameHashes.end(), (ending.m_numMwmId, mwmNameHash));

    NumMwmId const id = std::distance(mwmNameHashes.begin(), it);
    WriteToSink(sink, id);
  };

  for (auto const & [segId, seg] : graph.m_segments)
  {
    WriteVarUint(sink, segId);

    WriteVarUint(sink, static_cast<uint64_t>(std::lround(seg.m_weight * kDouble2Int)));

    writeSegEnding(seg.m_start);
    writeSegEnding(seg.m_end);
  }
}

// static
template <class Source>
void CrossBorderGraphSerializer::Deserialize(CrossBorderGraph & graph, Source & src,
                                             std::shared_ptr<NumMwmIds> numMwmIds)
{
  Header header;
  header.Deserialize(src);

  graph.m_mwms.reserve(header.m_numRegions);
  graph.m_segments.reserve(header.m_numRoads);

  std::map<uint32_t, NumMwmId> hashToMwmId;

  numMwmIds->ForEachId([&](NumMwmId id)
  {
    std::string const & region = numMwmIds->GetFile(id).GetName();
    uint32_t const mwmNameHash = Hash(region);
    // Triggering of this check in runtime means that the latest built mwm set differs from
    // the previous one ("classic" mwm set which is constant for many years). The solution is to
    // replace current hash function Hash() and rebuild World.mwm.
    CHECK(hashToMwmId.emplace(mwmNameHash, id).second, (id, region, mwmNameHash));
  });

  std::set<uint32_t> mwmNameHashes;

  for (size_t i = 0; i < header.m_numRegions; ++i)
  {
    auto const mwmNameHash = ReadPrimitiveFromSource<uint32_t>(src);
    mwmNameHashes.emplace(mwmNameHash);
  }

  auto readSegEnding = [&](CrossBorderSegmentEnding & ending)
  {
    double const lat = Uint32ToDouble(ReadPrimitiveFromSource<uint32_t>(src), ms::LatLon::kMinLat, ms::LatLon::kMaxLat,
                                      kBitsForDouble);
    double const lon = Uint32ToDouble(ReadPrimitiveFromSource<uint32_t>(src), ms::LatLon::kMinLon, ms::LatLon::kMaxLon,
                                      kBitsForDouble);
    ending.m_point = LatLonWithAltitude(ms::LatLon(lat, lon), geometry::kDefaultAltitudeMeters);

    NumMwmId index = ReadPrimitiveFromSource<uint16_t>(src);
    CHECK_LESS(index, mwmNameHashes.size(), ());

    auto it = mwmNameHashes.begin();
    std::advance(it, index);

    auto const & mwmHash = *it;
    auto itHash = hashToMwmId.find(mwmHash);
    CHECK(itHash != hashToMwmId.end(), (mwmHash));

    ending.m_numMwmId = itHash->second;
  };

  for (size_t i = 0; i < header.m_numRoads; ++i)
  {
    CrossBorderSegment seg;
    RegionSegmentId segId;

    if (header.m_version == 0)
    {
      segId = ReadPrimitiveFromSource<RegionSegmentId>(src);
      seg.m_weight = static_cast<double>(ReadPrimitiveFromSource<uint32_t>(src));
    }
    else
    {
      segId = ReadVarUint<RegionSegmentId>(src);
      seg.m_weight = static_cast<double>(ReadVarUint<uint64_t>(src));
      seg.m_weight /= kDouble2Int;
    }

    readSegEnding(seg.m_start);
    readSegEnding(seg.m_end);

    graph.AddCrossBorderSegment(segId, seg);
  }
}

template <class Sink>
void CrossBorderGraphSerializer::Header::Serialize(Sink & sink) const
{
  WriteToSink(sink, m_version);
  WriteToSink(sink, m_numRegions);
  WriteToSink(sink, m_numRoads);
}

template <class Source>
void CrossBorderGraphSerializer::Header::Deserialize(Source & src)
{
  m_version = ReadPrimitiveFromSource<uint32_t>(src);
  m_numRegions = ReadPrimitiveFromSource<uint32_t>(src);
  m_numRoads = ReadPrimitiveFromSource<uint32_t>(src);
}
}  // namespace routing
