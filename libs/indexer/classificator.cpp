#include "indexer/classificator.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/tree_structure.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>

using std::string;

namespace
{
struct less_scales
{
  bool operator()(drule::Key const & l, int r) const { return l.m_scale < r; }
  bool operator()(int l, drule::Key const & r) const { return l < r.m_scale; }
  bool operator()(drule::Key const & l, drule::Key const & r) const { return l.m_scale < r.m_scale; }
};
}  // namespace

/////////////////////////////////////////////////////////////////////////////////////////
// ClassifObject implementation
/////////////////////////////////////////////////////////////////////////////////////////

ClassifObject * ClassifObject::AddImpl(string const & s)
{
  if (m_objs.empty())
    m_objs.reserve(30);

  m_objs.emplace_back(s);
  return &(m_objs.back());
}

ClassifObject * ClassifObject::Add(string const & s)
{
  ClassifObject * p = Find(s);
  return (p ? p : AddImpl(s));
}

ClassifObject * ClassifObject::Find(string const & s)
{
  for (auto & obj : m_objs)
    if (obj.m_name == s)
      return &obj;

  return nullptr;
}

void ClassifObject::AddDrawRule(drule::Key const & k)
{
  auto i = std::lower_bound(m_drawRules.begin(), m_drawRules.end(), k.m_scale, less_scales());
  for (; i != m_drawRules.end() && i->m_scale == k.m_scale; ++i)
    if (k == *i)
      return;  // already exists
  m_drawRules.insert(i, k);

  if (k.m_priority > m_maxOverlaysPriority && (k.m_type == drule::symbol || k.m_type == drule::caption ||
                                               k.m_type == drule::shield || k.m_type == drule::pathtext))
    m_maxOverlaysPriority = k.m_priority;
}

ClassifObjectPtr ClassifObject::BinaryFind(std::string_view const s) const
{
  auto const i = std::lower_bound(m_objs.begin(), m_objs.end(), s, LessName());
  if ((i == m_objs.end()) || ((*i).m_name != s))
    return {nullptr, 0};
  else
    return {&(*i), static_cast<size_t>(std::distance(m_objs.begin(), i))};
}

void ClassifObject::LoadPolicy::Start(size_t i)
{
  ClassifObject * p = Current();
  p->m_objs.emplace_back();

  base_type::Start(i);
}

void ClassifObject::LoadPolicy::EndChilds()
{
  ClassifObject * p = Current();
  ASSERT(p->m_objs.back().m_name.empty(), ());
  p->m_objs.pop_back();
}

void ClassifObject::Sort()
{
  sort(m_drawRules.begin(), m_drawRules.end(), less_scales());
  sort(m_objs.begin(), m_objs.end(), LessName());
  for (auto & obj : m_objs)
    obj.Sort();
}

void ClassifObject::Swap(ClassifObject & r)
{
  swap(m_name, r.m_name);
  swap(m_drawRules, r.m_drawRules);
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
    return nullptr;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// Classificator implementation
/////////////////////////////////////////////////////////////////////////////////////////

Classificator & classif()
{
  static Classificator c[MapStyleCount];
  MapStyle const mapStyle = GetStyleReader().GetCurrentStyle();
  return c[mapStyle];
}

namespace ftype
{
uint8_t const bits_count = 7;
uint8_t const levels_count = 4;
uint8_t const max_value = (1 << bits_count) - 1;

void set_value(uint32_t & type, uint8_t level, uint8_t value)
{
  level *= bits_count;  // level to bits

  uint32_t const m1 = uint32_t(max_value) << level;
  type &= (~m1);  // zero bits

  uint32_t const m2 = uint32_t(value) << level;
  type |= m2;  // set bits
}

uint8_t get_value(uint32_t type, uint8_t level)
{
  level *= bits_count;  // level to bits;

  uint32_t const m = uint32_t(max_value) << level;
  type &= m;  // leave only our bits

  type = type >> level;  // move to start
  ASSERT(type <= max_value, ("invalid output value", type));

  return uint8_t(type);  // conversion
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
  ASSERT(value <= max_value, ("invalid input value", value));

  uint8_t const cl = get_control_level(type);
  // to avoid warning in release
#ifdef RELEASE
  UNUSED_VALUE(levels_count);
#endif
  ASSERT(cl < levels_count, (cl));

  set_value(type, cl, value);

  // set control level
  set_value(type, cl + 1, 1);
}

uint8_t GetValue(uint32_t type, uint8_t level)
{
  return get_value(type, level);
}

void PopValue(uint32_t & type)
{
  uint8_t const cl = get_control_level(type);
  ASSERT_GREATER(cl, 0, ());

  // remove control level
  set_value(type, cl, 0);

  // set control level
  set_value(type, cl - 1, 1);
}

void TruncValue(uint32_t & type, uint8_t level)
{
  ASSERT_GREATER(level, 0, ());
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
}  // namespace ftype

namespace
{
class suitable_getter
{
  typedef std::vector<drule::Key> vec_t;
  typedef vec_t::const_iterator iter_t;

  vec_t const & m_rules;
  drule::KeysT & m_keys;

  void add_rule(int ft, iter_t i)
  {
    // Define which drule types are applicable to which feature geom types.
    static int const visible[3][drule::count_of_rules] = {
        //{ line, area, symbol, caption, circle, pathtext, waymarker, shield }, see drule::Key::rule_type_t
        {0, 0, 1, 1, 1, 0, 0, 0},  // fpoint
        {1, 0, 0, 0, 0, 1, 0, 1},  // fline
        {0, 1, 1, 1, 1, 0, 0, 0}  // farea (!!! different from IsDrawableLike(): here area feature can use point styles)
    };

    if (visible[ft][i->m_type] == 1)
      m_keys.push_back(*i);
  }

public:
  suitable_getter(vec_t const & rules, drule::KeysT & keys) : m_rules(rules), m_keys(keys) {}

  void find(int ft, int scale)
  {
    auto i = std::lower_bound(m_rules.begin(), m_rules.end(), scale, less_scales());
    while (i != m_rules.end() && i->m_scale == scale)
      add_rule(ft, i++);
  }
};
}  // namespace

void ClassifObject::GetSuitable(int scale, feature::GeomType gt, drule::KeysT & keys) const
{
  ASSERT(static_cast<int>(gt) >= 0 && static_cast<int>(gt) <= 2, ());

  // 2. Check visibility criterion for scale first.
  if (!m_visibility[scale])
    return;

  // find rules for 'scale'
  suitable_getter rulesGetter(m_drawRules, keys);
  rulesGetter.find(static_cast<int>(gt), scale);
}

bool ClassifObject::IsDrawable(int scale) const
{
  return (m_visibility[scale] && IsDrawableAny());
}

bool ClassifObject::IsDrawableAny() const
{
  return (m_visibility != VisibleMask() && !m_drawRules.empty());
}

bool ClassifObject::IsDrawableLike(feature::GeomType gt, bool emptyName) const
{
  ASSERT(static_cast<int>(gt) >= 0 && static_cast<int>(gt) <= 2, ());

  // check the very common criterion first
  if (!IsDrawableAny())
    return false;

  // Define which feature geom types can use which drule types for rendering.
  static int const visible[3][drule::count_of_rules] = {
      //{ line, area, symbol, caption, circle, pathtext, waymarker, shield }, see drule::Key::rule_type_t
      {0, 0, 1, 1, 1, 0, 0, 0},  // fpoint
      {1, 0, 0, 0, 0, 1, 0, 1},  // fline
      {0, 1, 0, 0, 0, 0, 0, 0}   // farea (!!! key difference with GetSuitable, see suitable_getter::add_rule())
  };

  for (auto const & k : m_drawRules)
  {
    ASSERT_LESS(k.m_type, drule::count_of_rules, ());

    // In case when feature name is empty we don't take into account caption drawing rules.
    if ((visible[static_cast<int>(gt)][k.m_type] == 1) &&
        (!emptyName || (k.m_type != drule::caption && k.m_type != drule::pathtext)))
    {
      return true;
    }
  }

  return false;
}

std::pair<int, int> ClassifObject::GetDrawScaleRange() const
{
  if (!IsDrawableAny())
    return {-1, -1};

  int const count = static_cast<int>(m_visibility.size());

  int left = -1;
  for (int i = 0; i < count; ++i)
    if (m_visibility[i])
    {
      left = i;
      break;
    }

  ASSERT_NOT_EQUAL(left, -1, ());

  int right = left;
  for (int i = count - 1; i > left; --i)
    if (m_visibility[i])
    {
      right = i;
      break;
    }

  return {left, right};
}

void Classificator::ReadClassificator(std::istream & s)
{
  ClassifObject::LoadPolicy policy(&m_root);
  tree::LoadTreeAsText(s, policy);

  m_root.Sort();

  m_coastType = GetTypeByPath({"natural", "coastline"});
  m_stubType = GetTypeByPath({"mapswithme"});
}

template <typename Iter>
uint32_t Classificator::GetTypeByPathImpl(Iter beg, Iter end) const
{
  ClassifObject const * p = GetRoot();

  uint32_t type = ftype::GetEmptyValue();

  while (beg != end)
  {
    ClassifObjectPtr ptr = p->BinaryFind(*beg++);
    if (!ptr)
      return INVALID_TYPE;

    ftype::PushValue(type, ptr.GetIndex());
    p = ptr.get();
  }

  return type;
}

uint32_t Classificator::GetTypeByPathSafe(std::vector<std::string_view> const & path) const
{
  return GetTypeByPathImpl(path.begin(), path.end());
}

uint32_t Classificator::GetTypeByPath(std::vector<std::string> const & path) const
{
  uint32_t const type = GetTypeByPathImpl(path.cbegin(), path.cend());
  ASSERT_NOT_EQUAL(type, INVALID_TYPE, (path));
  return type;
}

uint32_t Classificator::GetTypeByPath(std::vector<std::string_view> const & path) const
{
  uint32_t const type = GetTypeByPathImpl(path.cbegin(), path.cend());
  ASSERT_NOT_EQUAL(type, INVALID_TYPE, (path));
  return type;
}

uint32_t Classificator::GetTypeByPath(base::StringIL const & lst) const
{
  uint32_t const type = GetTypeByPathImpl(lst.begin(), lst.end());
  ASSERT_NOT_EQUAL(type, INVALID_TYPE, (lst));
  return type;
}

uint32_t Classificator::GetTypeByReadableObjectName(std::string const & name) const
{
  ASSERT(!name.empty(), ());
  return GetTypeByPathSafe(strings::Tokenize(name, "-"));
}

void Classificator::ReadTypesMapping(std::istream & s)
{
  m_mapping.Load(s);
}

void Classificator::Clear()
{
  ClassifObject("world").Swap(m_root);
  m_mapping.Clear();
}

template <class ToDo>
void Classificator::ForEachPathObject(uint32_t type, ToDo && toDo) const
{
  ClassifObject const * p = &m_root;
  uint8_t const level = ftype::GetLevel(type);
  for (uint8_t i = 0; i < level; ++i)
  {
    p = p->GetObject(ftype::GetValue(type, i));
    toDo(p);
  }
}

ClassifObject const * Classificator::GetObject(uint32_t type) const
{
  ClassifObject const * res = nullptr;
  ForEachPathObject(type, [&res](ClassifObject const * p) { res = p; });
  return res;
}

std::string Classificator::GetFullObjectName(uint32_t type) const
{
  std::string res;
  ForEachPathObject(type, [&res](ClassifObject const * p) { res = res + p->GetName() + '|'; });
  return res;
}

std::vector<std::string> Classificator::GetFullObjectNamePath(uint32_t type) const
{
  std::vector<std::string> res;
  ForEachPathObject(type, [&res](ClassifObject const * p) { res.push_back(p->GetName()); });
  return res;
}

string Classificator::GetReadableObjectName(uint32_t type) const
{
  string s = GetFullObjectName(type);
  // Remove ending dummy symbol.
  ASSERT(!s.empty(), ());
  s.pop_back();
  // Replace separator.
  replace(s.begin(), s.end(), '|', '-');
  return s;
}
