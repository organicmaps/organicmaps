#include "feature_visibility.hpp"
#include "classificator.hpp"
#include "feature.hpp"
#include "scales.hpp"

#include "../base/assert.hpp"

#include "../std/array.hpp"


namespace
{
  bool NeedProcessParent(ClassifObject const * p)
  {
    return false;
  }
}

template <class ToDo> typename ToDo::ResultType
Classificator::ProcessObjects(uint32_t type, ToDo & toDo) const
{
  typedef typename ToDo::ResultType ResultType;
  ResultType res = ResultType(); // default initialization

  ClassifObject const * p = &m_root;
  uint8_t i = 0;
  uint8_t v;

  // it's enough for now with our 3-level classificator
  array<ClassifObject const *, 8> path;

  // get objects route in hierarchy for type
  while (ftype::GetValue(type, i, v))
  {
    p = p->GetObject(v);
    if (p != 0)
    {
      path[i++] = p;
      toDo(p);
    }
    else
      break;
  }
  if (path.empty())
    return res;
  else
  {
    // process objects from child to root
    for (; i > 0; --i)
    {
      // process and stop find if needed
      if (toDo(path[i-1], res)) break;

      // no need to process parents
      if (!NeedProcessParent(path[i-1])) break;
    }
    return res;
  }
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
    ClassifObject::FeatureGeoType m_ft;
    drule::KeysT & m_keys;
#ifdef DEBUG
    string & m_name;
#endif

  public:
    DrawRuleGetter(int scale, feature::EGeomType ft,
                  drule::KeysT & keys
#ifdef DEBUG
                   , string & name
#endif
                   )
      : m_scale(scale), m_ft(ClassifObject::FeatureGeoType(ft)), m_keys(keys)
#ifdef DEBUG
    , m_name(name)
#endif
    {
    }

    typedef bool ResultType;

    void operator() (ClassifObject const * p)
    {
#ifdef DEBUG
      if (!m_name.empty()) m_name += '-';
      m_name += p->GetName();
#endif
    }
    bool operator() (ClassifObject const * p, bool & res)
    {
      res = true;
      p->GetSuitable(min(m_scale, scales::GetUpperStyleScale()), m_ft, m_keys);
      return false;
    }
  };
}

namespace
{
  class UselessCheckPatch
  {
    uint32_t m_oneway, m_highway;

  public:
    UselessCheckPatch(Classificator const & c)
    {
      vector<string> v;
      v.push_back("oneway");
      m_oneway = c.GetTypeByPath(v);

      v.clear();
      v.push_back("highway");
      m_highway = c.GetTypeByPath(v);
    }

    bool IsHighway(uint32_t t) const
    {
      ftype::TruncValue(t, 1);
      return (t == m_highway);
    }

    bool IsOneway(uint32_t t) const { return (t == m_oneway); }
  };
}

pair<int, bool> GetDrawRule(FeatureBase const & f, int level,
                            drule::KeysT & keys, string & names)
{
  feature::TypesHolder types(f);

  ASSERT ( keys.empty(), () );
  Classificator const & c = classif();

  static UselessCheckPatch patch(c);

  bool hasHighway = false;
  for (size_t i = 0; i < types.Size(); ++i)
    if (patch.IsHighway(types[i]))
    {
      hasHighway = true;
      break;
    }

  DrawRuleGetter doRules(level, types.GetGeoType(), keys
#ifdef DEBUG
                         , names
#endif
                         );
  for (size_t i = 0; i < types.Size(); ++i)
  {
    if (!hasHighway && patch.IsOneway(types[i]))
      continue;

    (void)c.ProcessObjects(types[i], doRules);
  }

  return make_pair(types.GetGeoType(), types.Has(c.GetCoastType()));
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
    ClassifObject::FeatureGeoType m_type;

  public:
    IsDrawableLikeChecker(FeatureGeoType type)
      : m_type(ClassifObject::FeatureGeoType(type))
    {
    }

    typedef bool ResultType;

    void operator() (ClassifObject const *) {}
    bool operator() (ClassifObject const * p, bool & res)
    {
      if (p->IsDrawableLike(m_type))
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
    ClassifObject::FeatureGeoType m_ft;
    bool m_arr[3];

  public:
    IsDrawableRulesChecker(int scale, feature::EGeomType ft, int rules)
      : m_scale(scale), m_ft(ClassifObject::FeatureGeoType(ft))
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

      for (size_t i = 0; i < keys.size(); ++i)
      {
        if ((m_arr[0] && keys[i].m_type == drule::caption) ||
            (m_arr[1] && keys[i].m_type == drule::pathtext) ||
            (m_arr[2] && keys[i].m_type == drule::symbol))
        {
          res = true;
          return true;
        }
      }

      return false;
    }
  };
}

bool IsDrawableAny(uint32_t type)
{
  return classif().GetObject(type)->IsDrawableAny();
}

bool IsDrawableLike(vector<uint32_t> const & types, FeatureGeoType ft)
{
  Classificator const & c = classif();

  IsDrawableLikeChecker doCheck(ft);
  for (size_t i = 0; i < types.size(); ++i)
    if (c.ProcessObjects(types[i], doCheck))
      return true;
  return false;
}

bool IsDrawableForIndex(FeatureBase const & f, int level)
{
  Classificator const & c = classif();

  feature::TypesHolder types(f);

  if (types.GetGeoType() == feature::GEOM_AREA && !types.Has(c.GetCoastType()))
    if (!scales::IsGoodForLevel(level, f.GetLimitRect()))
      return false;

  IsDrawableChecker doCheck(level);
  for (size_t i = 0; i < types.Size(); ++i)
    if (c.ProcessObjects(types[i], doCheck))
      return true;

  return false;
}

namespace
{
  class CheckNonDrawableType
  {
    Classificator & m_c;
    FeatureGeoType m_type;

  public:
    CheckNonDrawableType(FeatureGeoType ft)
      : m_c(classif()), m_type(ft)
    {
    }

    bool operator() (uint32_t t)
    {
      IsDrawableLikeChecker doCheck(m_type);
      // return true if need to delete
      return !m_c.ProcessObjects(t, doCheck);
    }
  };
}

bool RemoveNoDrawableTypes(vector<uint32_t> & types, FeatureGeoType ft)
{
  types.erase(remove_if(types.begin(), types.end(), CheckNonDrawableType(ft)), types.end());
  return !types.empty();
}

int GetMinDrawableScale(FeatureBase const & f)
{
  int const upBound = scales::GetUpperStyleScale();

  for (int level = 0; level <= upBound; ++level)
    if (feature::IsDrawableForIndex(f, level))
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

  for (size_t i = 0; i < types.Size(); ++i)
    AddRange(res, GetDrawableScaleRange(types[i]));

  return (res.first > res.second ? make_pair(-1, -1) : res);
}

namespace
{
  bool IsDrawableForRules(feature::TypesHolder const & types, int level, int rules)
  {
    Classificator const & c = classif();

    IsDrawableRulesChecker doCheck(level, types.GetGeoType(), rules);
    for (size_t i = 0; i < types.Size(); ++i)
      if (c.ProcessObjects(types[i], doCheck))
        return true;

    return false;
  }
}

pair<int, int> GetDrawableScaleRangeForRules(feature::TypesHolder const & types, int rules)
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

void TypeSetChecker::SetType(StringT * beg, StringT * end)
{
  m_type = classif().GetTypeByPath(vector<string>(beg, end));
  m_level = distance(beg, end);
}

bool TypeSetChecker::IsEqual(uint32_t type) const
{
  ftype::TruncValue(type, m_level);
  return (m_type == type);
}

}
