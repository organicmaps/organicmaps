#pragma once

#include "indexer/drawing_rule_def.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_decl.hpp"

#include "base/base.hpp"

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

class FeatureType;

namespace feature
{
  class TypesHolder;

  bool TypeIsUseful(uint32_t type);
  bool IsDrawableForIndex(FeatureType & ft, int level);
  bool IsDrawableForIndex(TypesHolder const & types, m2::RectD limitRect, int level);

  // The separation into ClassifOnly and GeometryOnly versions is needed to speed up
  // the geometrical index (see indexer/scale_index_builder.hpp).
  // Technically, the GeometryOnly version uses the classificator, but it only does
  // so when checking against coastlines.
  bool IsDrawableForIndexClassifOnly(TypesHolder const & types, int level);
  bool IsDrawableForIndexGeometryOnly(FeatureType & ft, int level);
  bool IsDrawableForIndexGeometryOnly(TypesHolder const & types, m2::RectD limitRect, int level);

  /// For FEATURE_TYPE_AREA need to have at least one area-filling type.
  bool IsDrawableLike(std::vector<uint32_t> const & types, EGeomType geomType);
  /// For FEATURE_TYPE_AREA removes line-drawing only types.
  bool RemoveUselessTypes(std::vector<uint32_t> & types, EGeomType geomType,
                          bool emptyName = false);
  //@}

  int GetMinDrawableScale(FeatureType & ft);
  int GetMinDrawableScale(TypesHolder const & types, m2::RectD limitRect);
  int GetMinDrawableScaleClassifOnly(TypesHolder const & types);

  /// @return [-1, -1] if range is not drawable
  //@{
  /// @name Get scale range when feature is visible.
  std::pair<int, int> GetDrawableScaleRange(uint32_t type);
  std::pair<int, int> GetDrawableScaleRange(TypesHolder const & types);
  bool IsVisibleInRange(uint32_t type, std::pair<int, int> const & scaleRange);

  /// @name Get scale range when feature's text or symbol is visible.
  enum
  {
    RULE_CAPTION = 1,
    RULE_PATH_TEXT = 2,
    RULE_ANY_TEXT = RULE_CAPTION | RULE_PATH_TEXT,
    RULE_SYMBOL = 4
  };

  std::pair<int, int> GetDrawableScaleRangeForRules(TypesHolder const & types, int rules);
  //@}

  /// @return (geometry type, is coastline)
  pair<int, bool> GetDrawRule(TypesHolder const & types, int level,
                              drule::KeysT & keys);
  void GetDrawRule(std::vector<uint32_t> const & types, int level, int geoType,
                   drule::KeysT & keys);
  void FilterRulesByRuntimeSelector(FeatureType & f, int zoomLevel, drule::KeysT & keys);

  /// Used to check whether user types belong to particular classificator set.
  class TypeSetChecker
  {
    uint32_t m_type;
    uint8_t m_level;

  public:
    explicit TypeSetChecker(std::initializer_list<char const *> const & lst);

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
    bool IsEqualV(std::vector<uint32_t> const & v) const
    {
      return IsEqualR(v.begin(), v.end());
    }
  };
}
