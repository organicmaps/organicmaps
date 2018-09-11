#pragma once

#include "indexer/mwm_set.hpp"

#include "coding/memory_region.hpp"

#include <cstdint>
#include <memory>

#include "3party/succinct/elias_fano.hpp"

class DataSource;

namespace routing
{
class CityRoadsLoader
{
public:
  CityRoadsLoader(DataSource const & dataSource, MwmSet::MwmId const & mwmId);

  bool HasCityRoads() const { return m_cityRoads.size() > 0; }
  bool IsCityRoad(uint32_t fid) const;

private:
  std::unique_ptr<CopiedMemoryRegion> m_cityRoadsRegion;
  succinct::elias_fano m_cityRoads;
};
}  // namespace routing
