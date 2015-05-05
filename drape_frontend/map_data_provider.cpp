#include "drape_frontend/map_data_provider.hpp"

namespace df
{

MapDataProvider::MapDataProvider(TReadIDsFn const & idsReader,
                                 TReadFeaturesFn const & featureReader,
                                 TResolveCountryFn const & countryResolver,
                                 TIsCountryLoadedFn const & isCountryLoadedFn)
  : m_featureReader(featureReader)
  , m_idsReader(idsReader)
  , m_countryResolver(countryResolver)
  , m_isCountryLoadedFn(isCountryLoadedFn)
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

storage::TIndex MapDataProvider::FindCountry(m2::PointF const & pt)
{
  return m_countryResolver(pt);
}

MapDataProvider::TIsCountryLoadedFn const & MapDataProvider::GetIsCountryLoadedFn() const
{
  return m_isCountryLoadedFn;
}

}
