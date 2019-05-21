#include "map/cross_reference_delegate.hpp"

#include "search/city_finder.hpp"

#include "indexer/data_source.hpp"
#include "indexer/ftypes_sponsored.hpp"

CrossReferenceDelegate::CrossReferenceDelegate(DataSource const & dataSource,
                                               search::CityFinder & cityFinder)
  : m_dataSource(dataSource)
  , m_cityFinder(cityFinder)
{
}

std::string CrossReferenceDelegate::GetCityOsmId(m2::PointD const & point)
{
  auto const featureId = m_cityFinder.GetCityFeatureID(point);

  if (!featureId.IsValid())
    return {};

  FeaturesLoaderGuard guard(m_dataSource, featureId.m_mwmId);
  auto feature = guard.GetOriginalFeatureByIndex(featureId.m_index);
  if (!feature)
    return {};

  if (ftypes::IsCrossReferenceCityChecker::Instance()(*feature))
    return feature->GetMetadata().Get(feature::Metadata::FMD_SPONSORED_ID);

  return {};
}
