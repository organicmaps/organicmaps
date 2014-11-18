#pragma once

#include "../indexer/feature.hpp"

#include "../geometry/rect2d.hpp"

#include "../std/function.hpp"

namespace df
{

class MapDataProvider
{
public:
  typedef function<void (FeatureID const &)> TReadIdCallback;
  typedef function<void (FeatureType const &)> TReadFeatureCallback;
  typedef function<void (TReadFeatureCallback const & , vector<FeatureID> const &)> TReadFeaturesFn;
  typedef function<void (TReadIdCallback const & , m2::RectD const &, int)> TReadIDsFn;

  MapDataProvider(TReadIDsFn const & idsReader, TReadFeaturesFn const & featureReader);

  void ReadFeaturesID(TReadIdCallback const & fn, m2::RectD const & r, int scale) const;
  void ReadFeatures(TReadFeatureCallback const & fn, vector<FeatureID> const & ids) const;

private:
  TReadFeaturesFn m_featureReader;
  TReadIDsFn m_idsReader;
};

}
