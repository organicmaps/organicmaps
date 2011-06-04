#pragma once

#include "drawing_rule_def.hpp"

#include "../base/base.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"


class FeatureBase;

namespace feature
{
  enum FeatureGeoType { FEATURE_TYPE_POINT = 0, FEATURE_TYPE_LINE, FEATURE_TYPE_AREA };

  bool IsDrawableAny(uint32_t type);
  bool IsDrawableLike(vector<uint32_t> const & type, FeatureGeoType ft);
  bool IsDrawableForIndex(FeatureBase const & f, int level);

  int MinDrawableScaleForFeature(FeatureBase const & f);
  pair<int, int> DrawableScaleRangeForText(FeatureBase const & f);

  int GetDrawRule(FeatureBase const & f, int level, vector<drule::Key> & keys, string & names);

  bool IsHighway(vector<uint32_t> const & types);

  bool IsCountry(uint32_t type);
  template <class IterT>
  inline bool IsCountry(IterT beg, IterT end)
  {
    while (beg != end)
    {
      if (IsCountry(*beg++))
        return true;
    }
    return false;
  }
}
