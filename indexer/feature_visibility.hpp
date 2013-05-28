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

  /// @note do not change this values. Should be equal with EGeomType.
  /// Used for checking visibility (by drawing style) for feature's geometry type
  /// (for Area - check only area type, but can draw symbol or caption).
  enum FeatureGeoType
  {
    FEATURE_TYPE_POINT = 0,
    FEATURE_TYPE_LINE = 1,
    FEATURE_TYPE_AREA = 2
  };

  bool IsDrawableAny(uint32_t type);
  bool IsDrawableForIndex(FeatureBase const & f, int level);

  /// @name Be carefull with FEATURE_TYPE_AREA.
  /// It's check only unique area styles, be it also can draw symbol or caption.
  //@{
  bool IsDrawableLike(vector<uint32_t> const & types, FeatureGeoType ft);
  bool RemoveNoDrawableTypes(vector<uint32_t> & types, FeatureGeoType ft);
  //@}

  int GetMinDrawableScale(FeatureBase const & f);

  /// @return [-1, -1] if range is not drawable
  //@{
  /// @name Get scale range when feature is visible.
  pair<int, int> GetDrawableScaleRange(uint32_t type);
  pair<int, int> GetDrawableScaleRange(TypesHolder const & types);

  /// @name Get scale range when feature's text or symbol is visible.
  enum
  {
    RULE_TEXT = 1, RULE_SYMBOL = 2
  };

  pair<int, int> GetDrawableScaleRangeForRules(TypesHolder const & types, int rules);
  pair<int, int> GetDrawableScaleRangeForRules(FeatureBase const & f, int rules);
  //@}

  /// @return (geometry type, is coastline)
  pair<int, bool> GetDrawRule(FeatureBase const & f, int level,
                              drule::KeysT & keys, string & names);

  /// Used to check whether user types belong to particular classificator set.
  class TypeSetChecker
  {
    uint32_t m_type;
    uint8_t m_level;

    typedef char const * StringT;
    void SetType(StringT * beg, StringT * end);

  public:
    /// Construct by classificator set name.
    //@{
    TypeSetChecker(StringT name) { SetType(&name, &name + 1); }
    TypeSetChecker(StringT arr[], size_t n) { SetType(arr, arr + n); }
    //@}

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
