#pragma once

#include "drawing_rule_def.hpp"

#include "../base/base.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"


class FeatureBase;

namespace feature
{
  class TypesHolder;

  // Note! do not change this values. Should be equal with EGeomType.
  enum FeatureGeoType {
    FEATURE_TYPE_POINT = 0,
    FEATURE_TYPE_LINE = 1,
    FEATURE_TYPE_AREA = 2
  };

  bool IsDrawableAny(uint32_t type);
  bool IsDrawableLike(vector<uint32_t> const & type, FeatureGeoType ft);
  bool IsDrawableForIndex(FeatureBase const & f, int level);

  int GetMinDrawableScale(FeatureBase const & f);

  /// @return [-1, -1] if no any text exists
  //@{
  /// @name Get scale range when feature is visible.
  pair<int, int> GetDrawableScaleRange(uint32_t type);
  pair<int, int> GetDrawableScaleRange(TypesHolder const & types);

  /// @name Get scale range when feature's text is visible.
  pair<int, int> GetDrawableScaleRangeForText(TypesHolder const & types);
  pair<int, int> GetDrawableScaleRangeForText(FeatureBase const & f);
  //@}

  /// @return (geometry type, is coastline)
  pair<int, bool> GetDrawRule(FeatureBase const & f, int level,
                              vector<drule::Key> & keys, string & names);

  bool IsHighway(vector<uint32_t> const & types);
  //bool IsJunction(vector<uint32_t> const & types);

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
