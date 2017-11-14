#include "search/city_finder.hpp"

#include "indexer/feature_decl.hpp"

using namespace std;

namespace search
{
CityFinder::CityFinder(Index const & index)
    : m_unusedBoundaries(index)
    , m_unusedCache(m_cancellable)
    , m_finder(index, m_unusedBoundaries, m_unusedCache)
{
}

string CityFinder::GetCityName(m2::PointD const & p, int8_t lang)
{
  string city;
  m_finder.GetLocality(
      p, [&](LocalityItem const & item) { item.GetSpecifiedOrDefaultName(lang, city); });
  return city;
}

FeatureID CityFinder::GetCityFeatureID(m2::PointD const & p)
{
  FeatureID id;
  m_finder.GetLocality(p, [&id](LocalityItem const & item) { id = item.m_id; });
  return id;
}
}  // namespace search
