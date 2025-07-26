#pragma once

#include "routing/city_roads.hpp"

#include "coding/succinct_mapper.hpp"
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
struct CityRoadsHeader
{
  template <typename Sink>
  void Serialize(Sink & sink) const
  {
    WriteToSink(sink, m_version);
    WriteToSink(sink, m_endianness);
    WriteToSink(sink, m_dataSize);
  }

  template <typename Source>
  void Deserialize(Source & src)
  {
    m_version = ReadPrimitiveFromSource<uint16_t>(src);
    m_endianness = ReadPrimitiveFromSource<uint16_t>(src);
    m_dataSize = ReadPrimitiveFromSource<uint32_t>(src);
  }

  uint16_t m_version = 0;
  // Field |m_endianness| is reserved for endianness of the section.
  uint16_t m_endianness = 0;
  uint32_t m_dataSize = 0;
};

static_assert(sizeof(CityRoadsHeader) == 8, "Wrong header size of city_roads section.");

class CityRoadsSerializer
{
public:
  CityRoadsSerializer() = delete;

  template <typename Sink>
  static void Serialize(Sink & sink, std::vector<uint32_t> && cityRoadFeatureIds)
  {
    CityRoadsHeader header;
    auto const startOffset = sink.Pos();
    header.Serialize(sink);

    std::sort(cityRoadFeatureIds.begin(), cityRoadFeatureIds.end());
    CHECK(adjacent_find(cityRoadFeatureIds.cbegin(), cityRoadFeatureIds.cend()) == cityRoadFeatureIds.cend(),
          ("City road feature ids should be unique."));
    succinct::elias_fano::elias_fano_builder builder(cityRoadFeatureIds.back() + 1, cityRoadFeatureIds.size());
    for (auto fid : cityRoadFeatureIds)
      builder.push_back(fid);

    coding::FreezeVisitor<Writer> visitor(sink);
    succinct::elias_fano(&builder).map(visitor);

    auto const endOffset = sink.Pos();
    header.m_dataSize = static_cast<uint32_t>(endOffset - startOffset - sizeof(CityRoadsHeader));

    sink.Seek(startOffset);
    header.Serialize(sink);
    sink.Seek(endOffset);

    LOG(LINFO, ("Serialized", cityRoadFeatureIds.size(), "road feature ids in cities. Size:", endOffset - startOffset,
                "bytes."));
  }

  template <typename Source>
  static void Deserialize(Source & src, std::unique_ptr<CopiedMemoryRegion> & cityRoadsRegion,
                          succinct::elias_fano & cityRoads)
  {
    CityRoadsHeader header;
    header.Deserialize(src);
    CHECK_EQUAL(header.m_version, 0, ());

    std::vector<uint8_t> data(header.m_dataSize);
    src.Read(data.data(), data.size());
    cityRoadsRegion = std::make_unique<CopiedMemoryRegion>(std::move(data));
    coding::MapVisitor visitor(cityRoadsRegion->ImmutableData());
    cityRoads.map(visitor);
  }
};
}  // namespace routing
