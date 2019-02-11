#pragma once

#include "search/categories_cache.hpp"
#include "search/locality_finder.hpp"

#include "geometry/point2d.hpp"

#include "base/cancellable.hpp"

#include <cstdint>
#include <string>

class DataSource;
struct FeatureID;

namespace search
{
class CityFinder
{
public:
  // TODO (@milchakov): consider to reuse locality finder from search
  // engine.  Otherwise, CityFinder won't benefit from approximated
  // cities boundaries.
  explicit CityFinder(DataSource const & dataSource);

  std::string GetCityName(m2::PointD const & p, int8_t lang);
  std::string GetCityReadableName(m2::PointD const & p);
  FeatureID GetCityFeatureID(m2::PointD const & p);

private:
  base::Cancellable m_cancellable;
  search::CitiesBoundariesTable m_unusedBoundaries;
  search::VillagesCache m_unusedCache;
  search::LocalityFinder m_finder;
};
}  // namespace search
