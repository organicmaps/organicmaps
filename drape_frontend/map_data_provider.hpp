#pragma once

#include "storage/index.hpp"
#include "storage/storage_defines.hpp"

#include "indexer/feature.hpp"
#include "geometry/rect2d.hpp"

#include "std/function.hpp"

namespace df
{

class MapDataProvider
{
public:
  template <typename T> using TReadCallback = function<void (T const &)>;
  using TReadFeaturesFn = function<void (TReadCallback<FeatureType> const & , vector<FeatureID> const &)>;
  using TReadIDsFn = function<void (TReadCallback<FeatureID> const & , m2::RectD const &, int)>;
  using TUpdateCountryIndexFn = function<void (storage::TIndex const & , m2::PointF const &)>;
  using TIsCountryLoadedFn = function<bool (m2::PointD const & pt)>;
  using TDownloadFn = function<void (storage::TIndex const & countryIndex)>;

  MapDataProvider(TReadIDsFn const & idsReader,
                  TReadFeaturesFn const & featureReader,
                  TUpdateCountryIndexFn const & countryIndexUpdater,
                  TIsCountryLoadedFn const & isCountryLoadedFn,
                  TDownloadFn const & downloadMapHandler,
                  TDownloadFn const & downloadMapRoutingHandler,
                  TDownloadFn const & downloadRetryHandler);

  void ReadFeaturesID(TReadCallback<FeatureID> const & fn, m2::RectD const & r, int scale) const;
  void ReadFeatures(TReadCallback<FeatureType> const & fn, vector<FeatureID> const & ids) const;

  void UpdateCountryIndex(storage::TIndex const & currentIndex, m2::PointF const & pt);
  TIsCountryLoadedFn const & GetIsCountryLoadedFn() const;

  TDownloadFn const & GetDownloadMapHandler() const;
  TDownloadFn const & GetDownloadMapRoutingHandler() const;
  TDownloadFn const & GetDownloadRetryHandler() const;

private:
  TReadFeaturesFn m_featureReader;
  TReadIDsFn m_idsReader;
  TUpdateCountryIndexFn m_countryIndexUpdater;
  TIsCountryLoadedFn m_isCountryLoadedFn;
  TDownloadFn m_downloadMapHandler;
  TDownloadFn m_downloadMapRoutingHandler;
  TDownloadFn m_downloadRetryHandler;
};

}
