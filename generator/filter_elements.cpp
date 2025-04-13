#include "generator/filter_elements.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace
{
template <typename T>
class Set
{
public:
  bool Add(T const & key)
  {
    if (Contains(key))
      return false;

    m_vec.push_back(key);
    return true;
  }

  bool Contains(T const & key) const
  {
    return base::IsExist(m_vec, key);
  }

private:
  std::vector<T> m_vec;
};
}  // namespace

namespace generator
{
bool FilterData::IsMatch(Tags const & elementTags, Tags const & tags)
{
  return base::AllOf(tags, [&](OsmElement::Tag const & t) {
    auto const it = base::FindIf(elementTags, [&](OsmElement::Tag const & tag) {
      return tag.m_key == t.m_key;
    });
    return it == std::end(elementTags) ? false : t.m_value == "*" || it->m_value == t.m_value;
  });
}

void FilterData::AddSkippedId(uint64_t id)
{
  m_skippedIds.insert(id);
}

void FilterData::AddSkippedTags(Tags const & tags)
{
  m_rulesStorage.push_back(tags);
  for (auto const & t : tags)
    m_skippedTags.emplace(t.m_key, m_rulesStorage.back());
}

bool FilterData::NeedSkipWithId(uint64_t id) const
{
  return m_skippedIds.find(id) != std::end(m_skippedIds);
}

bool FilterData::NeedSkipWithTags(Tags const & tags) const
{
  Set<Tags const *> s;
  for (auto const & tag : tags)
  {
    auto const range = m_skippedTags.equal_range(tag.m_key);
    for (auto it = range.first; it != range.second; ++it)
    {
      Tags const & t = it->second;
      if (s.Add(&t) && IsMatch(tags, t))
        return true;
    }
  }

  return false;
}

bool FilterElements::ParseSection(json_t * json, FilterData & fdata)
{
  if (!json_is_object(json))
    return false;

  char const * key = nullptr;
  json_t * value = nullptr;
  json_object_foreach(json, key, value)
  {
    if (std::strcmp("ids", key) == 0 && !ParseIds(value, fdata))
      return false;
    else if (std::strcmp("tags", key) == 0 && !ParseTags(value, fdata))
      return false;
  }

  return true;
}

bool FilterElements::ParseIds(json_t * json, FilterData & fdata)
{
  if (!json_is_array(json))
    return false;

  size_t const sz = json_array_size(json);
  for (size_t i = 0; i < sz; ++i)
  {
    auto const * o = json_array_get(json, i);
    if (!json_is_integer(o))
      return false;

    auto const val = json_integer_value(o);
    if (val < 0)
      return false;

    fdata.AddSkippedId(static_cast<uint64_t>(val));
  }

  return true;
}

bool FilterElements::ParseTags(json_t * json, FilterData & fdata)
{
  if (!json_is_array(json))
    return false;

  size_t const sz = json_array_size(json);
  for (size_t i = 0; i < sz; ++i)
  {
    auto * o = json_array_get(json, i);
    if (!json_is_object(o))
      return false;

    char const * key = nullptr;
    json_t * value = nullptr;
    FilterData::Tags tags;
    json_object_foreach(o, key, value)
    {
      if (!json_is_string(value))
        return false;

      auto const val = json_string_value(value);
      tags.emplace_back(key, val);
    }

    if (!tags.empty())
      fdata.AddSkippedTags(tags);
  }

  return true;
}

FilterElements::FilterElements(std::string const & filename)
  : m_filename(filename)
{
  std::ifstream stream(m_filename);
  std::string str((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  if (!ParseString(str))
    LOG(LERROR, ("Cannot parse file", m_filename));
}

std::shared_ptr<FilterInterface> FilterElements::Clone() const
{
  return std::make_shared<FilterElements>(m_filename);
}

bool FilterElements::IsAccepted(OsmElement const & element) const
{
  return !NeedSkip(element);
}

bool FilterElements::NeedSkip(OsmElement const & element) const
{
  switch (element.m_type)
  {
  case OsmElement::EntityType::Node: return NeedSkip(element, m_nodes);
  case OsmElement::EntityType::Way: return NeedSkip(element, m_ways);
  case OsmElement::EntityType::Relation: return NeedSkip(element, m_relations);
  default: return false;
  }
}

bool FilterElements::NeedSkip(OsmElement const & element,  FilterData const & fdata) const
{
  return fdata.NeedSkipWithId(element.m_id) || fdata.NeedSkipWithTags(element.Tags());
}

bool FilterElements::ParseString(std::string const & str)
{
  base::Json json(str);
  if (!json_is_object(json.get()))
    return false;

  char const * key = nullptr;
  json_t * value = nullptr;
  json_object_foreach(json.get(), key, value)
  {
    bool res = true;
    if (std::strcmp("node", key) == 0)
      res = ParseSection(value, m_nodes);
    else if (std::strcmp("way", key) == 0)
      res = ParseSection(value, m_ways);
    else if (std::strcmp("relation", key) == 0)
      res = ParseSection(value, m_relations);

    if (!res)
      return false;
  }

  return true;
}
}  // namespace generator
