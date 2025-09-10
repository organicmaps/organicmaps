#include "search/city_finder.hpp"

#include "indexer/feature_decl.hpp"

namespace search
{
CityFinder::CityFinder(DataSource const & dataSource)
  : m_unusedBoundaries(dataSource)
  , m_unusedCache(m_cancellable)
  , m_finder(dataSource, m_unusedBoundaries, m_unusedCache)
{}

std::string CityFinder::GetCityName(m2::PointD const & p, int8_t lang)
{
  std::string_view city;
  if (auto loc = m_finder.GetBestLocality(p))
    loc->GetSpecifiedOrDefaultName(lang, city);
  return std::string(city);
}

std::string CityFinder::GetCityReadableName(m2::PointD const & p)
{
  std::string city;
  if (auto loc = m_finder.GetBestLocality(p))
    loc->GetReadableName(city);
  return city;
}

FeatureID CityFinder::GetCityFeatureID(m2::PointD const & p)
{
  FeatureID id;
  if (auto loc = m_finder.GetBestLocality(p))
    id = loc->m_id;
  return id;
}
}  // namespace search
