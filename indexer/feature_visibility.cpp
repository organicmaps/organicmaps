#include "indexer/feature_visibility.hpp"
#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/scales.hpp"

#include "base/assert.hpp"

#include "std/array.hpp"


template <class ToDo> typename ToDo::ResultType
Classificator::ProcessObjects(uint32_t type, ToDo & toDo) const
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

pair<int, bool> GetDrawRule(FeatureBase const & f, int level,
                            drule::KeysT & keys)
{
  TypesHolder types(f);

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

  /// Add here all exception classificator types: needed for algorithms,
  /// but don't have drawing rules.
  bool TypeAlwaysExists(uint32_t t, EGeomType g = GEOM_UNDEFINED)
  {
    static const uint32_t s1 = classif().GetTypeByPath({ "junction", "roundabout" });
    static const uint32_t s2 = classif().GetTypeByPath({ "hwtag" });

    if (g == GEOM_LINE || g == GEOM_UNDEFINED)
    {
      if (s1 == t)
        return true;

      ftype::TruncValue(t, 1);
      if (s2 == t)
        return true;
    }

    return false;
  }
}

bool IsDrawableAny(uint32_t type)
{
  return (TypeAlwaysExists(type) || classif().GetObject(type)->IsDrawableAny());
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

bool IsDrawableForIndex(FeatureBase const & f, int level)
{
  return IsDrawableForIndexGeometryOnly(f, level) && IsDrawableForIndexClassifOnly(f, level);
}

bool IsDrawableForIndexGeometryOnly(FeatureBase const & f, int level)
{
  Classificator const & c = classif();

  static uint32_t const buildingPartType = c.GetTypeByPath({"building:part"});

  TypesHolder const types(f);

  if (types.GetGeoType() == GEOM_AREA
      && !types.Has(c.GetCoastType()) && !types.Has(buildingPartType)
      && !scales::IsGoodForLevel(level, f.GetLimitRect()))
    return false;

  return true;
}

bool IsDrawableForIndexClassifOnly(FeatureBase const & f, int level)
{
  Classificator const & c = classif();

  TypesHolder const types(f);

  IsDrawableChecker doCheck(level);
  for (uint32_t t : types)
    if (c.ProcessObjects(t, doCheck))
      return true;

  return false;
}

bool RemoveNoDrawableTypes(vector<uint32_t> & types, EGeomType geomType, bool emptyName)
{
  Classificator const & c = classif();

  types.erase(remove_if(types.begin(), types.end(), [&] (uint32_t t)
  {
   if (TypeAlwaysExists(t, geomType))
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

int GetMinDrawableScale(FeatureBase const & f)
{
  int const upBound = scales::GetUpperStyleScale();

  for (int level = 0; level <= upBound; ++level)
    if (IsDrawableForIndex(f, level))
      return level;

  return -1;
}

int GetMinDrawableScaleClassifOnly(FeatureBase const & f)
{
  int const upBound = scales::GetUpperStyleScale();

  for (int level = 0; level <= upBound; ++level)
    if (IsDrawableForIndexClassifOnly(f, level))
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

pair<int, int> GetDrawableScaleRangeForRules(FeatureBase const & f, int rules)
{
  return GetDrawableScaleRangeForRules(TypesHolder(f), rules);
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
