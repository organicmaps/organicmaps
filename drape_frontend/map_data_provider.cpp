#include "drape_frontend/map_data_provider.hpp"

#include "base/assert.hpp"

#include <utility>

namespace df
{
MapDataProvider::MapDataProvider(TReadIDsFn && idsReader,
                                 TReadFeaturesFn && featureReader,
                                 TFilterFeatureFn && filterFeatureFn,
                                 TIsCountryLoadedByNameFn && isCountryLoadedByNameFn,
                                 TUpdateCurrentCountryFn && updateCurrentCountryFn)
  : m_isCountryLoadedByName(std::move(isCountryLoadedByNameFn))
  , m_featureReader(std::move(featureReader))
  , m_idsReader(std::move(idsReader))
  , m_filterFeature(std::move(filterFeatureFn))
  , m_updateCurrentCountry(std::move(updateCurrentCountryFn))
{
  CHECK(m_isCountryLoadedByName != nullptr, ());
  CHECK(m_featureReader != nullptr, ());
  CHECK(m_idsReader != nullptr, ());
  CHECK(m_filterFeature != nullptr, ());
  CHECK(m_updateCurrentCountry != nullptr, ());
}

void MapDataProvider::ReadFeaturesID(TReadCallback<FeatureID const> const & fn, m2::RectD const & r,
                                     int scale) const
{
  m_idsReader(fn, r, scale);
}

void MapDataProvider::ReadFeatures(TReadCallback<FeatureType> const & fn,
                                   std::vector<FeatureID> const & ids) const
{
  m_featureReader(fn, ids);
}

MapDataProvider::TFilterFeatureFn const & MapDataProvider::GetFilter() const
{
  return m_filterFeature;
}

MapDataProvider::TUpdateCurrentCountryFn const & MapDataProvider::UpdateCurrentCountryFn() const
{
  return m_updateCurrentCountry;
}
}  // namespace df
