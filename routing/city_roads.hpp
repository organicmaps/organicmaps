#pragma once

#include "indexer/mwm_set.hpp"

#include "coding/memory_region.hpp"
#include "coding/reader.hpp"

#include <memory>

#include "3party/succinct/elias_fano.hpp"

namespace routing
{
class CityRoads
{
public:
  bool HaveCityRoads() const { return m_cityRoads.size() > 0; }
  /// \returns true if |fid| is a feature id of a road (for cars, bicycles or pedestrians) in city
  /// or town.
  /// \note if there's no section with city roads returns false. That means for maps without
  /// city roads section features are considered as features outside cities.
  bool IsCityRoad(uint32_t fid) const;

  using ReaderT = ModelReaderPtr;
  void Load(ReaderT const & reader);

private:
  std::unique_ptr<CopiedMemoryRegion> m_cityRoadsRegion;
  // |m_cityRoads| contains true for feature ids which are roads in cities.
  succinct::elias_fano m_cityRoads;
};

std::unique_ptr<CityRoads> LoadCityRoads(MwmSet::MwmHandle const & handle);
}  // namespace routing
