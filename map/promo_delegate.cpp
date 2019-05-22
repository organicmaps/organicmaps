#include "map/promo_delegate.hpp"

#include "search/city_finder.hpp"

#include "indexer/data_source.hpp"
#include "indexer/ftypes_sponsored.hpp"

PromoDelegate::PromoDelegate(DataSource const & dataSource, search::CityFinder & cityFinder)
  : m_dataSource(dataSource), m_cityFinder(cityFinder)
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

  if (ftypes::IsPromoCatalogChecker::Instance()(*feature))
    return feature->GetMetadata().Get(feature::Metadata::FMD_SPONSORED_ID);

  return {};
}
