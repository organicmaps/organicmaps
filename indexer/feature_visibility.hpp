#pragma once

#include "drawing_rule_def.hpp"

#include "../base/base.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"

class FeatureBase;

namespace feature
{
  enum feature_geo_t { fpoint = 0, fline, farea };

  bool IsDrawableAny(uint32_t type);
  bool IsDrawableLike(vector<uint32_t> const & type, feature_geo_t ft);
  bool IsDrawableForIndex(FeatureBase const & f, int level);

  int MinDrawableScaleForFeature(FeatureBase const & f);
  int MinDrawableScaleForText(FeatureBase const & f);

  int GetDrawRule(FeatureBase const & f, int level, vector<drule::Key> & keys, string & names);

  bool IsHighway(vector<uint32_t> const & types);
  bool IsCountry(uint32_t type);
}
