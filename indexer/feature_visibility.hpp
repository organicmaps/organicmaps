#pragma once

#include "drawing_rule_def.hpp"

#include "../base/base.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"

class Feature;

namespace feature
{
  enum feature_geo_t { fpoint = 0, fline, farea };

  bool IsDrawableAny(uint32_t type);
  bool IsDrawableLike(vector<uint32_t> const & type, feature_geo_t ft);
  bool IsDrawableForIndex(Feature const & f, int level);
  uint32_t MinDrawableScaleForFeature(Feature const & f);


  int GetDrawRule(Feature const & f, int level, vector<drule::Key> & keys, string & names);
}
