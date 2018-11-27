#include "indexer/feature_visibility.hpp"

#include "indexer/classificator.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/scales.hpp"

#include "base/assert.hpp"

#include <array>

using namespace std;

template <typename ToDo>
typename ToDo::ResultType Classificator::ProcessObjects(uint32_t type, ToDo & toDo) const
{
  typedef typename ToDo::ResultType ResultType;
  ResultType res = ResultType(); // default initialization

  ClassifObject const * p = GetObject(type);
  if (p != &m_root)
  {
    ASSERT(p, ());
    toDo(p, res);
  }
  return res;
}

ClassifObject const * Classificator::GetObject(uint32_t type) const
{
  ClassifObject const * p = &m_root;
  uint8_t i = 0;

  // get the final ClassifObject
  uint8_t v;
  while (ftype::GetValue(type, i, v))
  {
    ++i;
    p = p->GetObject(v);
  }

  return p;
}

string Classificator::GetFullObjectName(uint32_t type) const
{
  ClassifObject const * p = &m_root;
  uint8_t i = 0;
  string s;

  // get the final ClassifObject
  uint8_t v;
  while (ftype::GetValue(type, i, v))
  {
    ++i;
    p = p->GetObject(v);
    s = s + p->GetName() + '|';
  }

  return s;
}

namespace feature
{

namespace
{
  class DrawRuleGetter
  {
    int m_scale;
    EGeomType m_ft;
    drule::KeysT & m_keys;

  public:
    DrawRuleGetter(int scale, EGeomType ft, drule::KeysT & keys)
      : m_scale(scale), m_ft(ft), m_keys(keys)
    {
    }

    typedef bool ResultType;

    void operator() (ClassifObject const *)
    {
    }
    bool operator() (ClassifObject const * p, bool & res)
    {
      res = true;
      p->GetSuitable(min(m_scale, scales::GetUpperStyleScale()), m_ft, m_keys);
      return false;
    }
  };
}

pair<int, bool> GetDrawRule(TypesHolder const & types, int level,
                            drule::KeysT & keys)
{
  ASSERT ( keys.empty(), () );
  Classificator const & c = classif();

  DrawRuleGetter doRules(level, types.GetGeoType(), keys);
  for (uint32_t t : types)
    (void)c.ProcessObjects(t, doRules);

  return make_pair(types.GetGeoType(), types.Has(c.GetCoastType()));
}

void GetDrawRule(vector<uint32_t> const & types, int level, int geoType,
                 drule::KeysT & keys)

{
  ASSERT ( keys.empty(), () );
  Classificator const & c = classif();

  DrawRuleGetter doRules(level, EGeomType(geoType), keys);

  for (uint32_t t : types)
    (void)c.ProcessObjects(t, doRules);
}

void FilterRulesByRuntimeSelector(FeatureType & f, int zoomLevel, drule::KeysT & keys)
{
  keys.erase_if([&f, zoomLevel](drule::Key const & key)->bool
  {
    drule::BaseRule const * const rule = drule::rules().Find(key);
    if (rule == nullptr)
      return true;
    return !rule->TestFeature(f, zoomLevel);
  });
}

namespace
{
  class IsDrawableChecker
  {
    int m_scale;

  public:
    IsDrawableChecker(int scale) : m_scale(scale) {}

    typedef bool ResultType;

    void operator() (ClassifObject const *) {}
    bool operator() (ClassifObject const * p, bool & res)
    {
      if (p->IsDrawable(m_scale))
      {
        res = true;
        return true;
      }
      return false;
    }
  };

  class IsDrawableLikeChecker
  {
    EGeomType m_geomType;
    bool m_emptyName;

  public:
    IsDrawableLikeChecker(EGeomType geomType, bool emptyName = false)
      : m_geomType(geomType), m_emptyName(emptyName)
    {
    }

    typedef bool ResultType;

    void operator() (ClassifObject const *) {}
    bool operator() (ClassifObject const * p, bool & res)
    {
      if (p->IsDrawableLike(m_geomType, m_emptyName))
      {
        res = true;
        return true;
      }
      return false;
    }
  };

  class IsDrawableRulesChecker
  {
    int m_scale;
    EGeomType m_ft;
    bool m_arr[3];

  public:
    IsDrawableRulesChecker(int scale, EGeomType ft, int rules)
      : m_scale(scale), m_ft(ft)
    {
      m_arr[0] = rules & RULE_CAPTION;
      m_arr[1] = rules & RULE_PATH_TEXT;
      m_arr[2] = rules & RULE_SYMBOL;
    }

    typedef bool ResultType;

    void operator() (ClassifObject const *) {}
    bool operator() (ClassifObject const * p, bool & res)
    {
      drule::KeysT keys;
      p->GetSuitable(m_scale, m_ft, keys);

      for (auto const & k : keys)
      {
        if ((m_arr[0] && k.m_type == drule::caption) ||
            (m_arr[1] && k.m_type == drule::pathtext) ||
            (m_arr[2] && k.m_type == drule::symbol))
        {
          res = true;
          return true;
        }
      }

      return false;
    }
  };

  bool HasRoutingExceptionType(uint32_t t)
  {
    static uint32_t const s = classif().GetTypeByPath({"route", "shuttle_train"});
    return s == t;
  }

  /// Add here all exception classificator types: needed for algorithms,
  /// but don't have drawing rules.
  bool TypeAlwaysExists(uint32_t type, EGeomType g = GEOM_UNDEFINED)
  {
    if (!classif().IsTypeValid(type))
      return false;

    static uint32_t const roundabout = classif().GetTypeByPath({"junction", "roundabout"});
    static uint32_t const internet = classif().GetTypeByPath({"internet_access"});
    static uint32_t const sponsored = classif().GetTypeByPath({"sponsored"});
    // Reserved for custom event processing, i.e. fc2018.
    // static uint32_t const event = classif().GetTypeByPath({"event" });

    if (g == GEOM_LINE || g == GEOM_UNDEFINED)
    {
      if (roundabout == type)
        return true;

      if (HasRoutingExceptionType(type))
        return true;
    }

    ftype::TruncValue(type, 1);
    if (g != GEOM_LINE)
    {
      if (sponsored == type || internet == type)
        return true;

      // Reserved for custom event processing, i.e. fc2018.
      //if (event == type)
      //  return true;
    }

    return false;
  }

  /// Add here all exception classificator types: needed for algorithms,
  /// but don't have drawing rules.
  bool IsUsefulNondrawableType(uint32_t type, EGeomType g = GEOM_UNDEFINED)
  {
    if (!classif().IsTypeValid(type))
      return false;

    if (TypeAlwaysExists(type, g))
      return true;

    static uint32_t const hwtag = classif().GetTypeByPath({"hwtag"});
    static uint32_t const psurface = classif().GetTypeByPath({"psurface"});
    static uint32_t const wheelchair = classif().GetTypeByPath({"wheelchair"});
    static uint32_t const cuisine = classif().GetTypeByPath({"cuisine"});

    // Caching type length to exclude generic [wheelchair].
    uint8_t const typeLength = ftype::GetLevel(type);

    ftype::TruncValue(type, 1);
    if (g == GEOM_LINE || g == GEOM_UNDEFINED)
    {
      if (hwtag == type || psurface == type)
        return true;
    }

    if (wheelchair == type && typeLength == 2)
      return true;

    if (cuisine == type)
      return true;

    return false;
  }
}  // namespace

bool TypeIsUseful(uint32_t type)
{
  return IsUsefulNondrawableType(type) || classif().GetObject(type)->IsDrawableAny();
}

bool IsDrawableLike(vector<uint32_t> const & types, EGeomType geomType)
{
  Classificator const & c = classif();

  IsDrawableLikeChecker doCheck(geomType);
  for (uint32_t t : types)
    if (c.ProcessObjects(t, doCheck))
      return true;
  return false;
}

bool IsDrawableForIndex(FeatureType & ft, int level)
{
  return IsDrawableForIndexGeometryOnly(ft, level) &&
         IsDrawableForIndexClassifOnly(TypesHolder(ft), level);
}

bool IsDrawableForIndex(TypesHolder const & types, m2::RectD limitRect, int level)
{
  return IsDrawableForIndexGeometryOnly(types, limitRect, level) &&
         IsDrawableForIndexClassifOnly(types, level);
}

bool IsDrawableForIndexGeometryOnly(FeatureType & ft, int level)
{
  return IsDrawableForIndexGeometryOnly(TypesHolder(ft),
                                        ft.GetLimitRect(FeatureType::BEST_GEOMETRY), level);
}
bool IsDrawableForIndexGeometryOnly(TypesHolder const & types, m2::RectD limitRect, int level)
{
  Classificator const & c = classif();

  static uint32_t const buildingPartType = c.GetTypeByPath({"building:part"});

  if (types.GetGeoType() == GEOM_AREA && !types.Has(c.GetCoastType()) &&
      !types.Has(buildingPartType) && !scales::IsGoodForLevel(level, limitRect))
    return false;

  return true;
}

bool IsDrawableForIndexClassifOnly(TypesHolder const & types, int level)
{
  Classificator const & c = classif();
  IsDrawableChecker doCheck(level);
  for (uint32_t t : types)
  {
    if (TypeAlwaysExists(t) || c.ProcessObjects(t, doCheck))
      return true;
  }

  return false;
}

bool RemoveUselessTypes(vector<uint32_t> & types, EGeomType geomType, bool emptyName)
{
  Classificator const & c = classif();

  types.erase(remove_if(types.begin(), types.end(), [&] (uint32_t t)
  {
   if (IsUsefulNondrawableType(t, geomType))
     return false;

   IsDrawableLikeChecker doCheck(geomType, emptyName);
   if (c.ProcessObjects(t, doCheck))
     return false;

   // IsDrawableLikeChecker checks only unique area styles,
   // so we need to take into account point styles too.
   if (geomType == GEOM_AREA)
   {
     IsDrawableLikeChecker doCheck(GEOM_POINT, emptyName);
     if (c.ProcessObjects(t, doCheck))
       return false;
   }

   return true;
  }), types.end());

  return !types.empty();
}

int GetMinDrawableScale(FeatureType & ft)
{
  return GetMinDrawableScale(TypesHolder(ft), ft.GetLimitRect(FeatureType::BEST_GEOMETRY));
}

int GetMinDrawableScale(TypesHolder const & types, m2::RectD limitRect)
{
  int const upBound = scales::GetUpperStyleScale();

  for (int level = 0; level <= upBound; ++level)
    if (IsDrawableForIndex(types, limitRect, level))
      return level;

  return -1;
}

int GetMinDrawableScaleClassifOnly(TypesHolder const & types)
{
  int const upBound = scales::GetUpperStyleScale();

  for (int level = 0; level <= upBound; ++level)
    if (IsDrawableForIndexClassifOnly(types, level))
      return level;

  return -1;
}

namespace
{
  void AddRange(pair<int, int> & dest, pair<int, int> const & src)
  {
    if (src.first != -1)
    {
      ASSERT_GREATER(src.first, -1, ());
      ASSERT_GREATER(src.second, -1, ());

      dest.first = min(dest.first, src.first);
      dest.second = max(dest.second, src.second);

      ASSERT_GREATER(dest.first, -1, ());
      ASSERT_GREATER(dest.second, -1, ());
    }
  }

  class DoGetScalesRange
  {
    pair<int, int> m_scales;
  public:
    DoGetScalesRange() : m_scales(1000, -1000) {}
    typedef bool ResultType;

    void operator() (ClassifObject const *) {}
    bool operator() (ClassifObject const * p, bool & res)
    {
      res = true;
      AddRange(m_scales, p->GetDrawScaleRange());
      return false;
    }

    pair<int, int> GetScale() const
    {
      return (m_scales.first > m_scales.second ? make_pair(-1, -1) : m_scales);
    }
  };
}

pair<int, int> GetDrawableScaleRange(uint32_t type)
{
  DoGetScalesRange doGet;
  (void)classif().ProcessObjects(type, doGet);
  return doGet.GetScale();
}

pair<int, int> GetDrawableScaleRange(TypesHolder const & types)
{
  pair<int, int> res(1000, -1000);

  for (uint32_t t : types)
    AddRange(res, GetDrawableScaleRange(t));

  return (res.first > res.second ? make_pair(-1, -1) : res);
}

bool IsVisibleInRange(uint32_t type, pair<int, int> const & scaleRange)
{
  CHECK_LESS_OR_EQUAL(scaleRange.first, scaleRange.second, (scaleRange));
  if (TypeAlwaysExists(type))
    return true;

  Classificator const & c = classif();
  for (int scale = scaleRange.first; scale <= scaleRange.second; ++scale)
  {
    IsDrawableChecker doCheck(scale);
    if (c.ProcessObjects(type, doCheck))
      return true;
  }
  return false;
}

namespace
{
  bool IsDrawableForRules(TypesHolder const & types, int level, int rules)
  {
    Classificator const & c = classif();

    IsDrawableRulesChecker doCheck(level, types.GetGeoType(), rules);
    for (uint32_t t : types)
      if (c.ProcessObjects(t, doCheck))
        return true;

    return false;
  }
}

pair<int, int> GetDrawableScaleRangeForRules(TypesHolder const & types, int rules)
{
  int const upBound = scales::GetUpperStyleScale();
  int lowL = -1;
  for (int level = 0; level <= upBound; ++level)
  {
    if (IsDrawableForRules(types, level, rules))
    {
      lowL = level;
      break;
    }
  }

  if (lowL == -1)
    return make_pair(-1, -1);

  int highL = lowL;
  for (int level = upBound; level > lowL; --level)
  {
    if (IsDrawableForRules(types, level, rules))
    {
      highL = level;
      break;
    }
  }

  return make_pair(lowL, highL);
}

TypeSetChecker::TypeSetChecker(initializer_list<char const *> const & lst)
{
  m_type = classif().GetTypeByPath(lst);
  m_level = lst.size();
}

bool TypeSetChecker::IsEqual(uint32_t type) const
{
  ftype::TruncValue(type, m_level);
  return (m_type == type);
}

}   // namespace feature
