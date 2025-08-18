#pragma once

#include "storage/storage_defines.hpp"

#include "geometry/rect2d.hpp"
#include "indexer/feature.hpp"

#include <functional>
#include <string>
#include <vector>

namespace df
{
class MapDataProvider
{
public:
  template <typename T>
  using TReadCallback = std::function<void(T &)>;
  using TReadFeaturesFn = std::function<void(TReadCallback<FeatureType> const &, std::vector<FeatureID> const &)>;
  using TReadIDsFn = std::function<void(TReadCallback<FeatureID const> const &, m2::RectD const &, int)>;
  using TIsCountryLoadedFn = std::function<bool(m2::PointD const &)>;
  using TIsCountryLoadedByNameFn = std::function<bool(std::string_view)>;
  using TUpdateCurrentCountryFn = std::function<void(m2::PointD const &, int)>;

  MapDataProvider(TReadIDsFn && idsReader, TReadFeaturesFn && featureReader,
                  TIsCountryLoadedByNameFn && isCountryLoadedByNameFn,
                  TUpdateCurrentCountryFn && updateCurrentCountryFn);

  void ReadFeaturesID(TReadCallback<FeatureID const> const & fn, m2::RectD const & r, int scale) const;
  void ReadFeatures(TReadCallback<FeatureType> const & fn, std::vector<FeatureID> const & ids) const;

  TUpdateCurrentCountryFn const & UpdateCurrentCountryFn() const;

  TIsCountryLoadedByNameFn m_isCountryLoadedByName;

private:
  TReadFeaturesFn m_featureReader;
  TReadIDsFn m_idsReader;
  TUpdateCurrentCountryFn m_updateCurrentCountry;
};
}  // namespace df
