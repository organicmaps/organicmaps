#include "drape_frontend/map_data_provider.hpp"

namespace df
{

MapDataProvider::MapDataProvider(TReadIDsFn const & idsReader,
                                 TReadFeaturesFn const & featureReader,
                                 TIsCountryLoadedFn const & isCountryLoadedFn,
                                 TIsCountryLoadedByNameFn const & isCountryLoadedByNameFn,
                                 TUpdateCurrentCountryFn const & updateCurrentCountryFn)
  : m_featureReader(featureReader)
  , m_idsReader(idsReader)
  , m_isCountryLoaded(isCountryLoadedFn)
  , m_updateCurrentCountry(updateCurrentCountryFn)
  , m_isCountryLoadedByName(isCountryLoadedByNameFn)
{
}

void MapDataProvider::ReadFeaturesID(TReadCallback<FeatureID> const & fn, m2::RectD const & r, int scale) const
{
  m_idsReader(fn, r, scale);
}

void MapDataProvider::ReadFeatures(TReadCallback<FeatureType> const & fn, vector<FeatureID> const & ids) const
{
  m_featureReader(fn, ids);
}

MapDataProvider::TIsCountryLoadedFn const & MapDataProvider::GetIsCountryLoadedFn() const
{
  return m_isCountryLoaded;
}

MapDataProvider::TUpdateCurrentCountryFn const & MapDataProvider::UpdateCurrentCountryFn() const
{
  return m_updateCurrentCountry;
}

} // namespace df
