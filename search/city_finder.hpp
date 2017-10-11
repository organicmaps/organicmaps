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
  explicit CityFinder(Index const & index)
    : m_unusedCache(m_cancellable), m_finder(index, m_unusedCache)
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
  search::VillagesCache m_unusedCache;
  search::LocalityFinder m_finder;
};
}  // namespace search
