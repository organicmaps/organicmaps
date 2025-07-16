#pragma once

#include "indexer/drawing_rule_def.hpp"
#include "indexer/feature_decl.hpp"

#include <initializer_list>
#include <utility>
#include <vector>

class FeatureType;

namespace feature
{
class TypesHolder;

bool IsCategoryNondrawableType(uint32_t type);
bool IsUsefulType(uint32_t type);
bool IsDrawableForIndex(FeatureType & ft, int level);
bool IsDrawableForIndex(TypesHolder const & types, m2::RectD const & limitRect, int level);

// The separation into ClassifOnly and GeometryOnly versions is needed to speed up
// the geometrical index (see indexer/scale_index_builder.hpp).
// Technically, the GeometryOnly version uses the classificator, but it only does
// so when checking against coastlines.
// bool IsDrawableForIndexClassifOnly(TypesHolder const & types, int level);
bool IsDrawableForIndexGeometryOnly(FeatureType & ft, int level);
// bool IsDrawableForIndexGeometryOnly(TypesHolder const & types, m2::RectD const & limitRect, int level);

/// @name Generator check functions.
/// @{

/// Can object with \a types can be generated as \a geomType Feature.
/// Should have appropriate drawing rules or satisfy "IsUsefulStandaloneType".
bool CanGenerateLike(std::vector<uint32_t> const & types, GeomType geomType);

/// @return true, if at least one valid type remains.
bool RemoveUselessTypes(std::vector<uint32_t> & types, GeomType geomType, bool emptyName = false);
/// @}

int GetMinDrawableScale(FeatureType & ft);
int GetMinDrawableScale(TypesHolder const & types, m2::RectD const & limitRect);
// int GetMinDrawableScaleGeometryOnly(TypesHolder const & types, m2::RectD limitRect);
int GetMinDrawableScaleClassifOnly(TypesHolder const & types);

/// @return [-1, -1] if range is not drawable
/// @{
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
  RULE_SYMBOL = 4,

  RULE_LINE = 8,
};

std::pair<int, int> GetDrawableScaleRangeForRules(TypesHolder const & types, int rules);
/// @}

void GetDrawRule(TypesHolder const & types, int level, drule::KeysT & keys);
void GetDrawRule(std::vector<uint32_t> const & types, int level, GeomType geomType, drule::KeysT & keys);
void FilterRulesByRuntimeSelector(FeatureType & f, int zoomLevel, drule::KeysT & keys);

/// Used to check whether user types belong to particular classificator set.
class TypeSetChecker
{
  uint32_t m_type;
  uint8_t m_level;

public:
  explicit TypeSetChecker(std::initializer_list<char const *> const & lst);

  bool IsEqual(uint32_t type) const;
  template <class IterT>
  bool IsEqualR(IterT beg, IterT end) const
  {
    while (beg != end)
      if (IsEqual(*beg++))
        return true;
    return false;
  }
  bool IsEqualV(std::vector<uint32_t> const & v) const { return IsEqualR(v.begin(), v.end()); }
};
}  // namespace feature
