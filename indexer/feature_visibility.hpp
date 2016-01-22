#pragma once

#include "indexer/drawing_rule_def.hpp"
#include "indexer/feature_decl.hpp"

#include "base/base.hpp"

#include "std/vector.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/initializer_list.hpp"


class FeatureBase;

namespace feature
{
  class TypesHolder;

  bool IsDrawableAny(uint32_t type);
  bool IsDrawableForIndex(FeatureBase const & f, int level);

  // The separation into ClassifOnly and GeometryOnly versions is needed to speed up
  // the geometrical index (see indexer/scale_index_builder.hpp).
  // Technically, the GeometryOnly version uses the classificator, but it only does
  // so when checking against coastlines.
  bool IsDrawableForIndexClassifOnly(FeatureBase const & f, int level);
  bool IsDrawableForIndexGeometryOnly(FeatureBase const & f, int level);

  /// For FEATURE_TYPE_AREA need to have at least one area-filling type.
  bool IsDrawableLike(vector<uint32_t> const & types, EGeomType geomType);
  /// For FEATURE_TYPE_AREA removes line-drawing only types.
  bool RemoveNoDrawableTypes(vector<uint32_t> & types, EGeomType geomType, bool emptyName = false);
  //@}

  int GetMinDrawableScale(FeatureBase const & f);
  int GetMinDrawableScaleClassifOnly(FeatureBase const & f);

  /// @return [-1, -1] if range is not drawable
  //@{
  /// @name Get scale range when feature is visible.
  pair<int, int> GetDrawableScaleRange(uint32_t type);
  pair<int, int> GetDrawableScaleRange(TypesHolder const & types);

  /// @name Get scale range when feature's text or symbol is visible.
  enum
  {
    RULE_CAPTION = 1,
    RULE_PATH_TEXT = 2,
    RULE_ANY_TEXT = RULE_CAPTION | RULE_PATH_TEXT,
    RULE_SYMBOL = 4
  };

  pair<int, int> GetDrawableScaleRangeForRules(TypesHolder const & types, int rules);
  pair<int, int> GetDrawableScaleRangeForRules(FeatureBase const & f, int rules);
  //@}

  /// @return (geometry type, is coastline)
  pair<int, bool> GetDrawRule(TypesHolder const & types, int level,
                              drule::KeysT & keys);
  void GetDrawRule(vector<uint32_t> const & types, int level, int geoType,
                   drule::KeysT & keys);

  /// Used to check whether user types belong to particular classificator set.
  class TypeSetChecker
  {
    uint32_t m_type;
    uint8_t m_level;

  public:
    explicit TypeSetChecker(initializer_list<char const *> const & lst);

    bool IsEqual(uint32_t type) const;
    template <class IterT> bool IsEqualR(IterT beg, IterT end) const
    {
      while (beg != end)
      {
        if (IsEqual(*beg++))
          return true;
      }
      return false;
    }
    bool IsEqualV(vector<uint32_t> const & v) const
    {
      return IsEqualR(v.begin(), v.end());
    }
  };
}
