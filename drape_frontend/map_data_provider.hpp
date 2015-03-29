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
  using TReadIdCallback = function<void (FeatureID const &)>;
  using TReadFeatureCallback = function<void (FeatureType const &)>;
  using TReadFeaturesFn = function<void (TReadFeatureCallback const & , vector<FeatureID> const &)>;
  using TReadIDsFn = function<void (TReadIdCallback const & , m2::RectD const &, int)>;
  using TResolveCountryFn = function<storage::TIndex (m2::PointF const &)>;

  MapDataProvider(TReadIDsFn const & idsReader,
                  TReadFeaturesFn const & featureReader,
                  TResolveCountryFn const & countryResolver);

  void ReadFeaturesID(TReadIdCallback const & fn, m2::RectD const & r, int scale) const;
  void ReadFeatures(TReadFeatureCallback const & fn, vector<FeatureID> const & ids) const;

  storage::TIndex FindCountry(m2::PointF const & pt);

private:
  TReadFeaturesFn m_featureReader;
  TReadIDsFn m_idsReader;
  TResolveCountryFn m_countryResolver;
};

}
