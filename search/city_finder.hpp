#pragma once

#include "search/categories_cache.hpp"
#include "search/locality_finder.hpp"

#include "geometry/point2d.hpp"

#include "base/cancellable.hpp"

#include <cstdint>
#include <string>

struct FeatureID;

namespace search
{
class CityFinder
{
public:
  // TODO (@milchakov): consider to reuse locality finder from search
  // engine.  Otherwise, CityFinder won't benefit from approximated
  // cities boundaries.
  explicit CityFinder(Index const & index);

  std::string GetCityName(m2::PointD const & p, int8_t lang);
  FeatureID GetCityFeatureID(m2::PointD const & p);

private:
  my::Cancellable m_cancellable;
  search::CitiesBoundariesTable m_unusedBoundaries;
  search::VillagesCache m_unusedCache;
  search::LocalityFinder m_finder;
};
}  // namespace search
