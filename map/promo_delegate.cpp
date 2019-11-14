#include "map/promo_delegate.hpp"

#include "search/city_finder.hpp"

#include "indexer/data_source.hpp"
#include "indexer/ftypes_sponsored.hpp"

#include "base/string_utils.hpp"

PromoDelegate::PromoDelegate(DataSource const & dataSource, search::CityFinder & cityFinder,
                             std::shared_ptr<CatalogHeadersProvider> const & headersProvider)
  : m_dataSource(dataSource), m_cityFinder(cityFinder), m_headersProvider(headersProvider)
{
}

std::string PromoDelegate::GetCityId(m2::PointD const & point)
{
  auto const featureId = m_cityFinder.GetCityFeatureID(point);

  if (!featureId.IsValid())
    return {};

  FeaturesLoaderGuard guard(m_dataSource, featureId.m_mwmId);
  auto feature = guard.GetOriginalFeatureByIndex(featureId.m_index);
  if (!feature)
    return {};

  if (!ftypes::IsPromoCatalogChecker::Instance()(*feature))
    return {};

  if (!m_cities)
  {
    m_cities = std::make_unique<indexer::FeatureIdToGeoObjectIdOneWay>(m_dataSource);
    m_cities->Load();
  }

  base::GeoObjectId id;
  if (m_cities->GetGeoObjectId(feature->GetID(), id))
    return strings::to_string(id);

  return {};
}

platform::HttpClient::Headers PromoDelegate::GetHeaders()
{
  if (!m_headersProvider)
    return {};

  return m_headersProvider->GetHeaders();
}
