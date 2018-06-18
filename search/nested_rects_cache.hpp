#pragma once

#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"

#include "std/map.hpp"
#include "std/vector.hpp"

class DataSourceBase;

namespace search
{
class NestedRectsCache
{
public:
  explicit NestedRectsCache(DataSourceBase const & index);

  void SetPosition(m2::PointD const & position, int scale);

  double GetDistanceToFeatureMeters(FeatureID const & id) const;

  void Clear();

private:
  enum RectScale
  {
    RECT_SCALE_TINY,
    RECT_SCALE_SMALL,
    RECT_SCALE_MEDIUM,
    RECT_SCALE_LARGE,

    RECT_SCALE_COUNT
  };

  static double GetRadiusMeters(RectScale scale);

  void Update();

  DataSourceBase const & m_index;
  int m_scale;
  m2::PointD m_position;
  bool m_valid;

  using TFeatures = vector<uint32_t>;
  using TBucket = map<MwmSet::MwmId, TFeatures>;

  // Sorted lists of features.
  TBucket m_buckets[RECT_SCALE_COUNT];
};
}  // namespace search
