#include "drape_frontend/map_data_provider.hpp"

namespace df
{

MapDataProvider::MapDataProvider(TReadIDsFn const & idsReader,
                                 TReadFeaturesFn const & featureReader,
                                 TResolveCountryFn const & countryResolver,
                                 TIsCountryLoadedFn const & isCountryLoadedFn,
                                 TDownloadFn const & downloadMapHandler,
                                 TDownloadFn const & downloadMapRoutingHandler,
                                 TDownloadFn const & downloadRetryHandler)
  : m_featureReader(featureReader)
  , m_idsReader(idsReader)
  , m_countryResolver(countryResolver)
  , m_isCountryLoadedFn(isCountryLoadedFn)
  , m_downloadMapHandler(downloadMapHandler)
  , m_downloadMapRoutingHandler(downloadMapRoutingHandler)
  , m_downloadRetryHandler(downloadRetryHandler)
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

MapDataProvider::TDownloadFn const & MapDataProvider::GetDownloadMapHandler() const
{
  return m_downloadMapHandler;
}

MapDataProvider::TDownloadFn const & MapDataProvider::GetDownloadMapRoutingHandler() const
{
  return m_downloadMapRoutingHandler;
}

MapDataProvider::TDownloadFn const & MapDataProvider::GetDownloadRetryHandler() const
{
  return m_downloadRetryHandler;
}

}
