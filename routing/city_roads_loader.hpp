#pragma once

#include "indexer/mwm_set.hpp"

#include "coding/memory_region.hpp"
#include "coding/writer.hpp"

#include <cstdint>
#include <memory>
#include <vector>

#include "3party/succinct/elias_fano.hpp"

class DataSource;

namespace routing
{
struct CityRoadsHeader
{
  template <class Sink>
  void Serialize(Sink & sink) const
  {
    WriteToSink(sink, m_version);
    WriteToSink(sink, m_endianness);
    WriteToSink(sink, m_dataSize);
  }

  template <class Source>
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

class CityRoadsLoader
{
public:
  CityRoadsLoader(DataSource const & dataSource, MwmSet::MwmId const & mwmId);

  bool HasCityRoads() const { return m_cityRoads.size() > 0; }
  bool IsCityRoad(uint32_t fid) const { return m_cityRoads[fid]; }

private:
  std::unique_ptr<CopiedMemoryRegion> m_cityRoadsRegion;
  succinct::elias_fano m_cityRoads;
};
}  // namespace routing
