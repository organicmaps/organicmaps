#pragma once

#include "indexer/mwm_set.hpp"

#include "coding/memory_region.hpp"

#include <cstdint>
#include <memory>

#include "3party/succinct/elias_fano.hpp"

class DataSource;

namespace routing
{
class CityRoads
{
  friend bool LoadCityRoads(DataSource const & dataSource, MwmSet::MwmId const & mwmId,
                            CityRoads & cityRoads);

public:
  bool HasCityRoads() const { return m_cityRoads.size() > 0; }
  // Returns true if |fid| is a feature id of a road (for cars, bicycles or pedestrians) in city.
  bool IsCityRoad(uint32_t fid) const;

private:
  std::unique_ptr<CopiedMemoryRegion> m_cityRoadsRegion;
  // |m_cityRoads| contains true for feature ids which are roads in cities.
  succinct::elias_fano m_cityRoads;
};

bool LoadCityRoads(DataSource const & dataSource, MwmSet::MwmId const & mwmId, CityRoads & cityRoads);
}  // namespace routing
