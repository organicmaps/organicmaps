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
  using TResolveCountryFn = function<storage::TIndex (m2::PointF const &)>;
  using TIsCountryLoadedFn = function<bool (m2::PointD const & pt)>;

  MapDataProvider(TReadIDsFn const & idsReader,
                  TReadFeaturesFn const & featureReader,
                  TResolveCountryFn const & countryResolver,
                  TIsCountryLoadedFn const & isCountryLoadedFn);

  void ReadFeaturesID(TReadCallback<FeatureID> const & fn, m2::RectD const & r, int scale) const;
  void ReadFeatures(TReadCallback<FeatureType> const & fn, vector<FeatureID> const & ids) const;

  storage::TIndex FindCountry(m2::PointF const & pt);
  TIsCountryLoadedFn const & GetIsCountryLoadedFn() const;

private:
  TReadFeaturesFn m_featureReader;
  TReadIDsFn m_idsReader;
  TResolveCountryFn m_countryResolver;
  TIsCountryLoadedFn m_isCountryLoadedFn;
};

}
