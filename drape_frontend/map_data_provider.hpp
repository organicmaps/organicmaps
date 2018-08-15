#pragma once

#include "storage/index.hpp"
#include "storage/storage_defines.hpp"

#include "indexer/feature.hpp"
#include "geometry/rect2d.hpp"

#include <functional>
#include <vector>

namespace df
{
class MapDataProvider
{
public:
  template <typename T> using TReadCallback = std::function<void(T &)>;
  using TReadFeaturesFn = std::function<void(TReadCallback<FeatureType> const &,
                                             std::vector<FeatureID> const &)>;
  using TReadIDsFn = std::function<void(TReadCallback<FeatureID const> const &,
                                        m2::RectD const &, int)>;
  using TIsCountryLoadedFn = std::function<bool(m2::PointD const &)>;
  using TIsCountryLoadedByNameFn = std::function<bool(string const &)>;
  using TUpdateCurrentCountryFn = std::function<void(m2::PointD const &, int)>;
  using TFilterFeatureFn = std::function<bool(FeatureType &)>;

  MapDataProvider(TReadIDsFn && idsReader,
                  TReadFeaturesFn && featureReader,
                  TFilterFeatureFn && filterFeatureFn,
                  TIsCountryLoadedByNameFn && isCountryLoadedByNameFn,
                  TUpdateCurrentCountryFn && updateCurrentCountryFn);

  void ReadFeaturesID(TReadCallback<FeatureID const> const & fn, m2::RectD const & r,
                      int scale) const;
  void ReadFeatures(TReadCallback<FeatureType> const & fn, std::vector<FeatureID> const & ids) const;

  TFilterFeatureFn const & GetFilter() const;

  TUpdateCurrentCountryFn const & UpdateCurrentCountryFn() const;

  TIsCountryLoadedByNameFn m_isCountryLoadedByName;

private:
  TReadFeaturesFn m_featureReader;
  TReadIDsFn m_idsReader;
  TFilterFeatureFn m_filterFeature;
  TUpdateCurrentCountryFn m_updateCurrentCountry;
};
}  // namespace df
