#include "generator/filter_elements.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <glaze/json.hpp>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <optional>

namespace generator::filter_elements_json
{
using TagsRule = std::unordered_map<std::string, std::string>;

struct Section
{
  std::vector<int64_t> ids;
  std::vector<TagsRule> tags;
};

struct Config
{
  std::optional<Section> node;
  std::optional<Section> way;
  std::optional<Section> relation;
};
}  // namespace generator::filter_elements_json

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

  bool Contains(T const & key) const { return base::IsExist(m_vec, key); }

private:
  std::vector<T> m_vec;
};

bool ParseIds(std::vector<int64_t> const & ids, generator::FilterData & fdata)
{
  for (auto const id : ids)
  {
    if (id < 0)
      return false;

    fdata.AddSkippedId(static_cast<uint64_t>(id));
  }

  return true;
}

bool ParseTags(std::vector<generator::filter_elements_json::TagsRule> const & rules, generator::FilterData & fdata)
{
  for (auto const & rule : rules)
  {
    generator::FilterData::Tags tags;
    tags.reserve(rule.size());
    for (auto const & [key, value] : rule)
      tags.emplace_back(key, value);

    if (!tags.empty())
      fdata.AddSkippedTags(tags);
  }

  return true;
}

bool ParseSection(generator::filter_elements_json::Section const & section, generator::FilterData & fdata)
{
  return ParseIds(section.ids, fdata) && ParseTags(section.tags, fdata);
}
}  // namespace

namespace generator
{
bool FilterData::IsMatch(Tags const & elementTags, Tags const & tags)
{
  return base::AllOf(tags, [&](OsmElement::Tag const & t)
  {
    auto const it = base::FindIf(elementTags, [&](OsmElement::Tag const & tag) { return tag.m_key == t.m_key; });
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

FilterElements::FilterElements(std::string const & filename) : m_filename(filename)
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

bool FilterElements::NeedSkip(OsmElement const & element, FilterData const & fdata) const
{
  return fdata.NeedSkipWithId(element.m_id) || fdata.NeedSkipWithTags(element.Tags());
}

bool FilterElements::ParseString(std::string const & str)
{
  filter_elements_json::Config config;
  glz::opts constexpr opts{.error_on_unknown_keys = false, .error_on_missing_keys = false};
  if (auto const error = glz::read<opts>(config, str); error)
  {
    LOG(LERROR, ("Cannot parse filter config", m_filename, glz::format_error(error, str)));
    return false;
  }

  if (config.node && !ParseSection(*config.node, m_nodes))
    return false;
  if (config.way && !ParseSection(*config.way, m_ways))
    return false;
  if (config.relation && !ParseSection(*config.relation, m_relations))
    return false;

  return true;
}
}  // namespace generator
