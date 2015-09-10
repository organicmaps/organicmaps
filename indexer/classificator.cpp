#include "indexer/classificator.hpp"
#include "indexer/tree_structure.hpp"

#include "base/macros.hpp"
#include "base/logging.hpp"

#include "std/bind.hpp"
#include "std/algorithm.hpp"
#include "std/iterator.hpp"

namespace
{
  struct less_scales
  {
    bool operator() (drule::Key const & l, int r) const { return l.m_scale < r; }
    bool operator() (int l, drule::Key const & r) const { return l < r.m_scale; }
    bool operator() (drule::Key const & l, drule::Key const & r) const { return l.m_scale < r.m_scale; }
  };
}  // namespace

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
    if (k == m_drawRule[i])
      return;

  auto i = lower_bound(m_drawRule.begin(), m_drawRule.end(), k.m_scale, less_scales());
  while (i != m_drawRule.end() && i->m_scale == k.m_scale) ++i;
  m_drawRule.insert(i, k);
}

ClassifObjectPtr ClassifObject::BinaryFind(string const & s) const
{
  const_iter_t i = lower_bound(m_objs.begin(), m_objs.end(), s, less_name_t());
  if ((i == m_objs.end()) || ((*i).m_name != s))
    return ClassifObjectPtr(0, 0);
  else
    return ClassifObjectPtr(&(*i), distance(m_objs.begin(), i));
}

void ClassifObject::LoadPolicy::Start(size_t i)
{
  ClassifObject * p = Current();
  p->m_objs.push_back(ClassifObject());

  base_type::Start(i);
}

void ClassifObject::LoadPolicy::EndChilds()
{
  ClassifObject * p = Current();
  ASSERT ( p->m_objs.back().m_name.empty(), () );
  p->m_objs.pop_back();
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
  if (i < m_objs.size())
    return &(m_objs[i]);
  else
  {
    LOG(LINFO, ("Map contains object that has no classificator entry", i, m_name));
    return 0;
  }
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
    // to avoid warning in release
#ifdef RELEASE
    UNUSED_VALUE(levels_count);
#endif
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
        { 0, 0, 1, 1, 1, 0, 0, 0 },   // fpoint
        { 1, 0, 0, 0, 0, 1, 0, 1 },   // fline
        { 1, 1, 1, 1, 1, 0, 0, 0 }    // farea
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

void ClassifObject::GetSuitable(int scale, feature::EGeomType ft, drule::KeysT & keys) const
{
  ASSERT(ft >= 0 && ft <= 2, ());

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

bool ClassifObject::IsDrawableLike(feature::EGeomType ft) const
{
  ASSERT(ft >= 0 && ft <= 2, ());

  // check the very common criterion first
  if (!IsDrawableAny())
    return false;

  static const int visible[3][drule::count_of_rules] = {
    {0, 0, 1, 1, 1, 0, 0, 0},   // fpoint
    {1, 0, 0, 0, 0, 1, 0, 1},   // fline
    {0, 1, 0, 0, 0, 0, 0, 0}    // farea (!!! key difference with GetSuitable !!!)
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

  int const count = static_cast<int>(m_visibility.size());

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
  ClassifObject::LoadPolicy policy(&m_root);
  tree::LoadTreeAsText(s, policy);

  m_root.Sort();

  m_coastType = GetTypeByPath({ "natural", "coastline" });
}

void Classificator::SortClassificator()
{
  GetMutableRoot()->Sort();
}

template <class IterT> uint32_t Classificator::GetTypeByPathImpl(IterT beg, IterT end) const
{
  ClassifObject const * p = GetRoot();

  uint32_t type = ftype::GetEmptyValue();

  while (beg != end)
  {
    ClassifObjectPtr ptr = p->BinaryFind(*beg++);
    if (!ptr)
      return 0;

    ftype::PushValue(type, ptr.GetIndex());
    p = ptr.get();
  }

  return type;
}

uint32_t Classificator::GetTypeByPathSafe(vector<string> const & path) const
{
  return GetTypeByPathImpl(path.begin(), path.end());
}

uint32_t Classificator::GetTypeByPath(vector<string> const & path) const
{
  uint32_t const type = GetTypeByPathImpl(path.begin(), path.end());
  ASSERT_NOT_EQUAL(type, 0, (path));
  return type;
}

uint32_t Classificator::GetTypeByPath(initializer_list<char const *> const & lst) const
{
  uint32_t const type = GetTypeByPathImpl(lst.begin(), lst.end());
  ASSERT_NOT_EQUAL(type, 0, (lst));
  return type;
}

void Classificator::ReadTypesMapping(istream & s)
{
  m_mapping.Load(s);
}

void Classificator::Clear()
{
  ClassifObject("world").Swap(m_root);
  m_mapping.Clear();
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
