#pragma once

#include "drawing_rule_def.hpp"

#include "../base/base.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"


class FeatureBase;

namespace feature
{
  // Note! do not change this values. Should be equal with EGeomType.
  enum FeatureGeoType {
    FEATURE_TYPE_POINT = 0,
    FEATURE_TYPE_LINE = 1,
    FEATURE_TYPE_AREA = 2
  };

  bool IsDrawableAny(uint32_t type);
  bool IsDrawableLike(vector<uint32_t> const & type, FeatureGeoType ft);
  bool IsDrawableForIndex(FeatureBase const & f, int level);

  int MinDrawableScaleForFeature(FeatureBase const & f);
  pair<int, int> DrawableScaleRangeForText(FeatureBase const & f);

  /// @return (geometry type, is coastline)
  pair<int, bool> GetDrawRule(FeatureBase const & f, int level,
                              vector<drule::Key> & keys, string & names);

  bool IsHighway(vector<uint32_t> const & types);

  bool UsePopulationRank(uint32_t type);

  template <class IterT>
  inline bool UsePopulationRank(IterT beg, IterT end)
  {
    while (beg != end)
    {
      if (UsePopulationRank(*beg++))
        return true;
    }
    return false;
  }
}
