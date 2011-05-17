#include "feature_visibility.hpp"
#include "classificator.hpp"
#include "feature.hpp"
#include "scales.hpp"

#include "../base/assert.hpp"

#include "../std/array.hpp"

#include "../base/start_mem_debug.hpp"



namespace
{
  bool need_process_parent(ClassifObject const * p)
  {
    string const & n = p->GetName();
    // same as is_mark_key (@see osm2type.cpp)
    return (n == "bridge" || n == "junction" || n == "oneway" || n == "fee");
  }
}

template <class ToDo> typename ToDo::result_type
Classificator::ProcessObjects(uint32_t type, ToDo & toDo) const
{
  typedef typename ToDo::result_type res_t;
  res_t res = res_t(); // default initialization

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
      if (!need_process_parent(path[i-1])) break;
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
    s = s + p->GetName() + '-';
  }

  return s;
}

namespace feature
{

namespace
{
  class get_draw_rule
  {
    int m_scale;
    feature_geo_t m_ft;
    vector<drule::Key> & m_keys;
    string & m_name;

  public:
    get_draw_rule(int scale, feature_geo_t ft,
                  vector<drule::Key> & keys, string & name)
      : m_scale(scale), m_ft(ft), m_keys(keys), m_name(name)
    {
    }

    typedef bool result_type;

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
      p->GetSuitable(m_scale, ClassifObject::feature_t(m_ft), m_keys);
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

  get_draw_rule doRules(level, static_cast<feature_geo_t>(geoType), keys, names);
  for (int i = 0; i < types.m_size; ++i)
    (void)c.ProcessObjects(types.m_types[i], doRules);

  return geoType;
}

namespace
{
  class check_is_drawable
  {
    int m_scale;
  public:
    check_is_drawable(int scale) : m_scale(scale) {}

    typedef bool result_type;

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

  class check_is_drawable_like
  {
    ClassifObject::feature_t m_type;
  public:
    check_is_drawable_like(feature_geo_t type)
      : m_type(ClassifObject::feature_t(type))
    {
    }

    typedef bool result_type;

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
}

bool IsDrawableAny(uint32_t type)
{
  return classif().GetObject(type)->IsDrawableAny();
}

bool IsDrawableLike(vector<uint32_t> const & types, feature_geo_t ft)
{
  Classificator const & c = classif();

  check_is_drawable_like doCheck(ft);
  for (size_t i = 0; i < types.size(); ++i)
    if (c.ProcessObjects(types[i], doCheck))
      return true;
  return false;
}

bool IsDrawableForIndex(FeatureBase const & f, int level)
{
  if (f.GetFeatureType() == feature::GEOM_AREA)
    if (!scales::IsGoodForLevel(level, f.GetLimitRect()))
      return false;

  FeatureBase::GetTypesFn types;
  f.ForEachTypeRef(types);

  Classificator const & c = classif();

  check_is_drawable doCheck(level);
  for (int i = 0; i < types.m_size; ++i)
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
