#include "search/city_finder.hpp"

#include "indexer/feature_decl.hpp"

using namespace std;

namespace search
{
CityFinder::CityFinder(DataSource const & dataSource)
  : m_unusedBoundaries(dataSource)
  , m_unusedCache(m_cancellable)
  , m_finder(dataSource, m_unusedBoundaries, m_unusedCache)
{}

string CityFinder::GetCityName(m2::PointD const & p, int8_t lang)
{
  string_view city;
  m_finder.GetLocality(p, [&](LocalityItem const & item) { item.GetSpecifiedOrDefaultName(lang, city); });

  // Return string, because m_finder.GetLocality() is not persistent.
  return std::string(city);
}

string CityFinder::GetCityReadableName(m2::PointD const & p)
{
  string_view city;
  m_finder.GetLocality(p, [&](LocalityItem const & item) { item.GetReadableName(city); });

  // Return string, because m_finder.GetLocality() is not persistent.
  return std::string(city);
}

FeatureID CityFinder::GetCityFeatureID(m2::PointD const & p)
{
  FeatureID id;
  m_finder.GetLocality(p, [&id](LocalityItem const & item) { id = item.m_id; });
  return id;
}
}  // namespace search
