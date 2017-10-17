#pragma once

#include "search/categories_cache.hpp"
#include "search/locality_finder.hpp"

#include "geometry/point2d.hpp"

#include "base/cancellable.hpp"

#include <cstdint>

namespace search
{
class CityFinder
{
public:
  // TODO (@milchakov): consider to reuse locality finder from search
  // engine.  Otherwise, CityFinder won't benefit from approximated
  // cities boundaries.
  explicit CityFinder(Index const & index)
    : m_unusedBoundaries(index)
    , m_unusedCache(m_cancellable)
    , m_finder(index, m_unusedBoundaries, m_unusedCache)
  {
  }

  string GetCityName(m2::PointD const & p, int8_t lang)
  {
    string city;
    m_finder.GetLocality(
        p, [&](LocalityItem const & item) { item.GetSpecifiedOrDefaultName(lang, city); });
    return city;
  }

private:
  my::Cancellable m_cancellable;
  search::CitiesBoundariesTable m_unusedBoundaries;
  search::VillagesCache m_unusedCache;
  search::LocalityFinder m_finder;
};
}  // namespace search
