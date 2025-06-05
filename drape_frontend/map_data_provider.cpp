#include "drape_frontend/map_data_provider.hpp"

#include "base/assert.hpp"

#include <utility>

namespace df
{
MapDataProvider::MapDataProvider(TReadIDsFn && idsReader, TReadFeaturesFn && featureReader,
                                 TIsCountryLoadedByNameFn && isCountryLoadedByNameFn,
                                 TUpdateCurrentCountryFn && updateCurrentCountryFn)
  : m_isCountryLoadedByName(std::move(isCountryLoadedByNameFn))
  , m_featureReader(std::move(featureReader))
  , m_idsReader(std::move(idsReader))
  , m_updateCurrentCountry(std::move(updateCurrentCountryFn))
{
  CHECK(m_isCountryLoadedByName != nullptr, ());
  CHECK(m_featureReader != nullptr, ());
  CHECK(m_idsReader != nullptr, ());
  CHECK(m_updateCurrentCountry != nullptr, ());
}

void MapDataProvider::ReadFeaturesID(TReadCallback<FeatureID const> const & fn, m2::RectD const & r, int scale) const
{
  m_idsReader(fn, r, scale);
}

void MapDataProvider::ReadFeatures(TReadCallback<FeatureType> const & fn, std::vector<FeatureID> const & ids) const
{
  m_featureReader(fn, ids);
}

MapDataProvider::TUpdateCurrentCountryFn const & MapDataProvider::UpdateCurrentCountryFn() const
{
  return m_updateCurrentCountry;
}
}  // namespace df
