#include "classificator.hpp"
#include "tree_structure.hpp"
#include "scales.hpp"

#include "../coding/file_reader.hpp"

#include "../base/assert.hpp"

#include "../std/target_os.hpp"
#include "../std/bind.hpp"
#include "../std/algorithm.hpp"
#include "../std/iterator.hpp"
#include "../std/fstream.hpp"


/////////////////////////////////////////////////////////////////////////////////////////
// ClassifObject implementation
/////////////////////////////////////////////////////////////////////////////////////////

ClassifObject * ClassifObject::AddImpl(string const & s)
{
  if (m_objs.empty()) m_objs.reserve(30);

  m_objs.push_back(ClassifObject(s));
  return &(m_objs.back());
}

ClassifObject * ClassifObject::Add(string const & s)
{
  ClassifObject * p = Find(s);
  return (p ? p : AddImpl(s));
}

void ClassifObject::AddCriterion(string const & s)
{
  Add('[' + s + ']');
}

ClassifObject * ClassifObject::Find(string const & s)
{
  for (iter_t i = m_objs.begin(); i != m_objs.end(); ++i)
    if ((*i).m_name == s)
      return &(*i);

  return 0;
}

void ClassifObject::AddDrawRule(drule::Key const & k)
{
  for (size_t i = 0; i < m_drawRule.size(); ++i)
    if (k == m_drawRule[i]) return;

  m_drawRule.push_back(k);
}

bool ClassifObject::IsCriterion() const
{
  return (m_name[0] == '[');
}

ClassifObjectPtr ClassifObject::BinaryFind(string const & s) const
{
  const_iter_t i = lower_bound(m_objs.begin(), m_objs.end(), s, less_name_t());
  if ((i == m_objs.end()) || ((*i).m_name != s))
    return ClassifObjectPtr(0, 0);
  else
    return ClassifObjectPtr(&(*i), distance(m_objs.begin(), i));
}

void ClassifObject::SavePolicy::Serialize(ostream & s) const
{
  ClassifObject const * p = Current();
  for (size_t i = 0; i < p->m_drawRule.size(); ++i)
    s << p->m_drawRule[i].toString() << "  ";
}

void ClassifObject::LoadPolicy::Serialize(string const & s)
{
  ClassifObject * p = Current();

  // load drawing rule
  drule::Key key;
  key.fromString(s);

  // mark as visible in rule's scale
  p->m_visibility[key.m_scale] = true;

  // mark objects visible on higher zooms as visible on upperScale, to get them into .mwm file
  int const upperScale = scales::GetUpperScale();
  if (key.m_scale > upperScale)
    p->m_visibility[upperScale] = true;
}

void ClassifObject::LoadPolicy::Start(size_t i)
{
  ClassifObject * p = Current();
  p->m_objs.push_back(ClassifObject());

  base_type::Start(i);
}

namespace
{
  struct less_scales
  {
    bool operator() (drule::Key const & l, int r) const { return l.m_scale < r; }
    bool operator() (int l, drule::Key const & r) const { return l < r.m_scale; }
    bool operator() (drule::Key const & l, drule::Key const & r) const { return l.m_scale < r.m_scale; }
  };
}

void ClassifObject::LoadPolicy::EndChilds()
{
  ClassifObject * p = Current();
  ASSERT ( p->m_objs.back().m_name.empty(), () );
  p->m_objs.pop_back();
}

void ClassifObject::VisSavePolicy::Serialize(ostream & s) const
{
  ClassifObject const * p = Current();

  size_t const count = p->m_visibility.size();

  string str;
  str.resize(count);
  for (size_t i = 0; i < count; ++i)
    str[i] = p->m_visibility[i] ? '1' : '0';

  s << str << "  ";
}

void ClassifObject::VisLoadPolicy::Name(string const & name) const
{
  UNUSED_VALUE(name);
  // Assume that classificator doesn't changed for saved visibility.
  ASSERT_EQUAL ( name, Current()->m_name, () );
}

void ClassifObject::VisLoadPolicy::Serialize(string const & s)
{
  ClassifObject * p = Current();

  for (size_t i = 0; i < s.size(); ++i)
    p->m_visibility[i] = (s[i] == '1');
}

void ClassifObject::VisLoadPolicy::Start(size_t i)
{
  if (i < Current()->m_objs.size())
    base_type::Start(i);
  else
    m_stack.push_back(0); // dummy
}

void ClassifObject::Sort()
{
  sort(m_drawRule.begin(), m_drawRule.end(), less_scales());
  sort(m_objs.begin(), m_objs.end(), less_name_t());
  for_each(m_objs.begin(), m_objs.end(), bind(&ClassifObject::Sort, _1));
}

void ClassifObject::Swap(ClassifObject & r)
{
  swap(m_name, r.m_name);
  swap(m_drawRule, r.m_drawRule);
  swap(m_objs, r.m_objs);
  swap(m_visibility, r.m_visibility);
}

ClassifObject const * ClassifObject::GetObject(size_t i) const
{
  ASSERT_LESS ( i, m_objs.size(), (m_name) );
  return &(m_objs[i]);
}

void ClassifObject::ConcatChildNames(string & s) const
{
  s.clear();
  size_t const count = m_objs.size();
  for (size_t i = 0; i < count; ++i)
  {
    s += m_objs[i].GetName();
    if (i != count-1) s += '|';
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// Classificator implementation
/////////////////////////////////////////////////////////////////////////////////////////

Classificator & classif()
{
  static Classificator c;
  return c;
}

namespace ftype
{
  uint8_t const bits_count = 6;
  uint8_t const levels_count = 5;
  uint8_t const max_value = (1 << bits_count) - 1;

  void set_value(uint32_t & type, uint8_t level, uint8_t value)
  {
    level *= bits_count;  // level to bits

    uint32_t const m1 = uint32_t(max_value) << level;
    type &= (~m1);        // zero bits

    uint32_t const m2 = uint32_t(value) << level;
    type |= m2;           // set bits
  }

  uint8_t get_value(uint32_t type, uint8_t level)
  {
    level *= bits_count;     // level to bits;

    uint32_t const m = uint32_t(max_value) << level;
    type &= m;              // leave only our bits

    type = type >> level;   // move to start
    ASSERT ( type <= max_value, ("invalid output value", type) );

    return uint8_t(type);   // conversion
  }

  uint8_t get_control_level(uint32_t type)
  {
    uint8_t count = 0;
    while (type > 1)
    {
      type = type >> bits_count;
      ++count;
    }

    return count;
  }

  void PushValue(uint32_t & type, uint8_t value)
  {
    ASSERT ( value <= max_value, ("invalid input value", value) );

    uint8_t const cl = get_control_level(type);
    ASSERT ( cl < levels_count, (cl) );

    set_value(type, cl, value);

    // set control level
    set_value(type, cl+1, 1);
  }

  bool GetValue(uint32_t type, uint8_t level, uint8_t & value)
  {
    ASSERT ( level < levels_count, ("invalid input level", level) );

    if (level < get_control_level(type))
    {
      value = get_value(type, level);
      return true;
    }
    return false;
  }

  void PopValue(uint32_t & type)
  {
    uint8_t const cl = get_control_level(type);
    ASSERT_GREATER ( cl, 0, () );

    // remove control level
    set_value(type, cl, 0);

    // set control level
    set_value(type, cl-1, 1);
  }

  void TruncValue(uint32_t & type, uint8_t level)
  {
    ASSERT_GREATER ( level, 0, () );
    uint8_t cl = get_control_level(type);

    while (cl > level)
    {
      // remove control level
      set_value(type, cl, 0);

      --cl;

      // set control level
      set_value(type, cl, 1);
    }
  }

  uint8_t GetLevel(uint32_t type)
  {
    return get_control_level(type);
  }
}

namespace
{
  class suitable_getter
  {
    typedef vector<drule::Key> vec_t;
    typedef vec_t::const_iterator iter_t;

    vec_t const & m_rules;
    drule::KeysT & m_keys;

    bool m_added;

    void add_rule(int ft, iter_t i)
    {
      static const int visible[3][drule::count_of_rules] = {
        { 0, 0, 1, 1, 1, 0, 0 },   // fpoint
        { 1, 0, 0, 0, 0, 1, 0 },   // fline
        { 1, 1, 1, 1, 0, 0, 0 }    // farea
      };

      if (visible[ft][i->m_type] == 1)
      {
        m_keys.push_back(*i);
        m_added = true;
      }
    }

  public:
    suitable_getter(vec_t const & rules, drule::KeysT & keys)
      : m_rules(rules), m_keys(keys)
    {
    }

    void find(int ft, int scale)
    {
      iter_t i = lower_bound(m_rules.begin(), m_rules.end(), scale, less_scales());
      while (i != m_rules.end() && i->m_scale == scale)
        add_rule(ft, i++);
    }
  };
}

void ClassifObject::GetSuitable(int scale, FeatureGeoType ft, drule::KeysT & keys) const
{
  ASSERT ( ft <= FEATURE_TYPE_AREA, () );

  // 2. Check visibility criterion for scale first.
  if (!m_visibility[scale])
    return;

  // find rules for 'scale'
  suitable_getter rulesGetter(m_drawRule, keys);
  rulesGetter.find(ft, scale);
}

bool ClassifObject::IsDrawable(int scale) const
{
  return (m_visibility[scale] && IsDrawableAny());
}

bool ClassifObject::IsDrawableAny() const
{
  return (m_visibility != visible_mask_t() && !m_drawRule.empty());
}

bool ClassifObject::IsDrawableLike(FeatureGeoType ft) const
{
  // check the very common criterion first
  if (!IsDrawableAny())
    return false;

  ASSERT ( ft <= FEATURE_TYPE_AREA, () );

  static const int visible[3][drule::count_of_rules] = {
    {0, 0, 1, 1, 1, 0, 0},   // fpoint
    {1, 0, 0, 0, 0, 1, 0},   // fline
    {0, 1, 0, 0, 0, 0, 0}    // farea (!!! key difference with GetSuitable !!!)
  };

  for (size_t i = 0; i < m_drawRule.size(); ++i)
  {
    ASSERT ( m_drawRule[i].m_type < drule::count_of_rules, () );
    if (visible[ft][m_drawRule[i].m_type] == 1)
    {
      /// @todo Check if rule's scale is reachable according to m_visibility (see GetSuitable algorithm).
      return true;
    }
  }

  return false;
}

pair<int, int> ClassifObject::GetDrawScaleRange() const
{
  if (!IsDrawableAny())
    return make_pair(-1, -1);

  size_t const count = m_visibility.size();

  int left = -1;
  for (int i = 0; i < count; ++i)
    if (m_visibility[i])
    {
      left = i;
      break;
    }

  ASSERT_NOT_EQUAL ( left, -1, () );

  int right = left;
  for (int i = count-1; i > left; --i)
    if (m_visibility[i])
    {
      right = i;
      break;
    }

  return make_pair(left, right);
}

void Classificator::ReadClassificator(istream & s)
{
  m_root.Clear();

  ClassifObject::LoadPolicy policy(&m_root);
  tree::LoadTreeAsText(s, policy);

  m_root.Sort();

  char const * path[] = { "natural", "coastline" };
  m_coastType = GetTypeByPath(vector<string>(path, path + 2));
}

void Classificator::PrintClassificator(char const * fPath)
{
#ifndef OMIM_OS_BADA
  ofstream file(fPath);

  ClassifObject::SavePolicy policy(&m_root);
  tree::SaveTreeAsText(file, policy);

#else
  ASSERT ( false, ("PrintClassificator uses only in indexer_tool") );
#endif
}

void Classificator::SortClassificator()
{
  GetMutableRoot()->Sort();
}

uint32_t Classificator::GetTypeByPath(vector<string> const & path) const
{
  ClassifObject const * p = GetRoot();

  size_t i = 0;
  uint32_t type = ftype::GetEmptyValue();

  while (i < path.size())
  {
    ClassifObjectPtr ptr = p->BinaryFind(path[i]);
    ASSERT ( ptr, ("Invalid path in Classificator::GetTypeByPath", path) );

    ftype::PushValue(type, ptr.GetIndex());

    ++i;
    p = ptr.get();
  }

  return type;
}

void Classificator::ReadTypesMapping(istream & s)
{
  m_mapping.Load(s);
}

void Classificator::Clear()
{
  *this = Classificator();
}

string Classificator::GetReadableObjectName(uint32_t type) const
{
  string s = classif().GetFullObjectName(type);

  // remove ending dummy symbol
  ASSERT ( !s.empty(), () );
  s.resize(s.size()-1);

  // replace separator
  replace(s.begin(), s.end(), '|', '-');
  return s;
}
