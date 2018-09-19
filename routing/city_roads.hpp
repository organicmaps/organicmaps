#pragma once

#include "indexer/mwm_set.hpp"

#include "coding/memory_region.hpp"
#include "coding/reader.hpp"

#include <cstdint>
#include <memory>
#include <string>

#include "3party/succinct/elias_fano.hpp"

class DataSource;

namespace routing
{
class CityRoads
{
  friend void LoadCityRoads(std::string const & fileName, FilesContainerR::TReader const & reader,
                            CityRoads & cityRoads);

public:
  bool HaveCityRoads() const { return m_cityRoads.size() > 0; }
  /// \returns true if |fid| is a feature id of a road (for cars, bicycles or pedestrians) in city
  /// or town.
  /// \note if there's no section with city roads returns false. That means for maps without
  /// city roads section features are considered as features outside cities.
  bool IsCityRoad(uint32_t fid) const;

private:
  std::unique_ptr<CopiedMemoryRegion> m_cityRoadsRegion;
  // |m_cityRoads| contains true for feature ids which are roads in cities.
  succinct::elias_fano m_cityRoads;
};

std::unique_ptr<CityRoads> LoadCityRoads(DataSource const & dataSource, MwmSet::MwmHandle const & handle);
void LoadCityRoads(std::string const & fileName, FilesContainerR::TReader const & reader,
                   CityRoads & cityRoads);
}  // namespace routing
