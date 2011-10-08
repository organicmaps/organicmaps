#include "feature_visibility.hpp"
#include "classificator.hpp"
#include "feature.hpp"
#include "scales.hpp"

#include "../base/assert.hpp"

#include "../std/array.hpp"

#include "../base/start_mem_debug.hpp"


namespace
{
  bool NeedProcessParent(ClassifObject const * p)
  {
    string const & n = p->GetName();
    // same as is_mark_key (@see osm2type.cpp)
    return (n == "bridge" || n == "junction" || n == "oneway" || n == "fee");
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
    path[i++] = p;
    toDo(p);
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
    vector<drule::Key> & m_keys;
    string & m_name;

  public:
    DrawRuleGetter(int scale, feature::EGeomType ft,
                  vector<drule::Key> & keys, string & name)
      : m_scale(scale), m_ft(ClassifObject::FeatureGeoType(ft)), m_keys(keys), m_name(name)
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
      p->GetSuitable(m_scale, m_ft, m_keys);
      return false;
    }
  };
}

int GetDrawRule(FeatureBase const & f, int level, vector<drule::Key> & keys, string & names)
{
  feature::EGeomType const geoType = f.GetFeatureType();

  FeatureBase::GetTypesFn types;
  f.ForEachTypeRef(types);

  ASSERT ( keys.empty(), () );
  Classificator const & c = classif();

  DrawRuleGetter doRules(level, geoType, keys, names);
  for (size_t i = 0; i < types.m_size; ++i)
    (void)c.ProcessObjects(types.m_types[i], doRules);

  return geoType;
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

  class TextRulesChecker
  {
    int m_scale;
    ClassifObject::FeatureGeoType m_ft;

  public:
    TextRulesChecker(int scale, feature::EGeomType ft)
      : m_scale(scale), m_ft(ClassifObject::FeatureGeoType(ft))
    {
    }

    typedef bool ResultType;

    void operator() (ClassifObject const *) {}
    bool operator() (ClassifObject const * p, bool & res)
    {
      vector<drule::Key> keys;
      p->GetSuitable(m_scale, m_ft, keys);

      for (size_t i = 0; i < keys.size(); ++i)
        if (keys[i].m_type == drule::caption || keys[i].m_type == drule::pathtext)
        {
          res = true;
          return true;
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
  static uint32_t const coastType = c.GetCoastType();

  FeatureBase::GetTypesFn types;
  f.ForEachTypeRef(types);

  if (f.GetFeatureType() == feature::GEOM_AREA && !types.Has(coastType))
    if (!scales::IsGoodForLevel(level, f.GetLimitRect()))
      return false;

  IsDrawableChecker doCheck(level);
  for (size_t i = 0; i < types.m_size; ++i)
    if (c.ProcessObjects(types.m_types[i], doCheck))
      return true;

  return false;
}

int MinDrawableScaleForFeature(FeatureBase const & f)
{
  int const upBound = scales::GetUpperScale();

  for (int level = 0; level <= upBound; ++level)
    if (feature::IsDrawableForIndex(f, level))
      return level;

  return -1;
}

namespace 
{
  bool IsDrawable(FeatureBase::GetTypesFn const & types, int level, EGeomType geomType)
  {
    Classificator const & c = classif();

    TextRulesChecker doCheck(level, geomType);
    for (size_t i = 0; i < types.m_size; ++i)
      if (c.ProcessObjects(types.m_types[i], doCheck))
        return true;

    return false;
  }
}

pair<int, int> DrawableScaleRangeForText(FeatureBase const & f)
{
  FeatureBase::GetTypesFn types;
  f.ForEachTypeRef(types);

  feature::EGeomType const geomType = f.GetFeatureType();

  int const upBound = scales::GetUpperScale();
  int lowL = -1;
  for (int level = 0; level <= upBound; ++level)
  {
    if (IsDrawable(types, level, geomType))
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
    if (IsDrawable(types, level, geomType))
    {
      highL = level;
      break;
    }
  }

  return make_pair(lowL, highL);
}

bool IsHighway(vector<uint32_t> const & types)
{
  ClassifObject const * pRoot = classif().GetRoot();

  for (size_t i = 0; i < types.size(); ++i)
  {
    uint8_t v;
    CHECK(ftype::GetValue(types[i], 0, v), (types[i]));
    {
      if (pRoot->GetObject(v)->GetName() == "highway")
        return true;
    }
  }

  return false;
}

bool IsCountry(uint32_t type)
{
  class checker_t
  {
  public:
    uint32_t m_type;

    checker_t()
    {
      vector<string> vec;
      vec.push_back("place");
      vec.push_back("country");
      m_type = classif().GetTypeByPath(vec);
    }
  };

  static checker_t checker;
  return (type == checker.m_type);
}

}
