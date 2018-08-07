#include "drape_frontend/map_data_provider.hpp"

namespace df
{
MapDataProvider::MapDataProvider(TReadIDsFn const & idsReader,
                                 TReadFeaturesFn const & featureReader,
                                 TIsCountryLoadedByNameFn const & isCountryLoadedByNameFn,
                                 TUpdateCurrentCountryFn const & updateCurrentCountryFn)
  : m_isCountryLoadedByName(isCountryLoadedByNameFn)
  , m_featureReader(featureReader)
  , m_idsReader(idsReader)
  , m_updateCurrentCountry(updateCurrentCountryFn)
{}

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

MapDataProvider::TUpdateCurrentCountryFn const & MapDataProvider::UpdateCurrentCountryFn() const
{
  return m_updateCurrentCountry;
}
}  // namespace df
