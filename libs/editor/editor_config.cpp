#include "editor/editor_config.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <cstring>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>

namespace editor
{
namespace
{
using EType = feature::Metadata::EType;

// TODO(mgsergio): It would be nice to have this map generated from editor.config.
static std::unordered_map<std::string, EType> const kNamesToFMD = {
    {"opening_hours", EType::FMD_OPEN_HOURS},
    {"phone", EType::FMD_PHONE_NUMBER},
    {"fax", EType::FMD_FAX_NUMBER},
    {"stars", EType::FMD_STARS},
    {"operator", EType::FMD_OPERATOR},
    {"website", EType::FMD_WEBSITE},
    {"contact_facebook", EType::FMD_CONTACT_FACEBOOK},
    {"contact_instagram", EType::FMD_CONTACT_INSTAGRAM},
    {"contact_twitter", EType::FMD_CONTACT_TWITTER},
    {"contact_vk", EType::FMD_CONTACT_VK},
    {"contact_line", EType::FMD_CONTACT_LINE},
    {"internet", EType::FMD_INTERNET},
    {              "ele",               EType::FMD_ELE},
    // {"", EType::FMD_TURN_LANES},
    // {"", EType::FMD_TURN_LANES_FORWARD},
    // {"", EType::FMD_TURN_LANES_BACKWARD},
    {"email", EType::FMD_EMAIL},
    {"postcode", EType::FMD_POSTCODE},
    {        "wikipedia",         EType::FMD_WIKIPEDIA},
    // {"", EType::FMD_MAXSPEED},
    {"flats", EType::FMD_FLATS},
    {"height", EType::FMD_HEIGHT},
    // {"", EType::FMD_MIN_HEIGHT},
    {"denomination", EType::FMD_DENOMINATION},
    {"building:levels", EType::FMD_BUILDING_LEVELS},
    {"level", EType::FMD_LEVEL},
    {"drive_through", EType::FMD_DRIVE_THROUGH},
    {"website_menu", EType::FMD_WEBSITE_MENU},
    {"self_service", EType::FMD_SELF_SERVICE},
    {  "outdoor_seating",   EType::FMD_OUTDOOR_SEATING}
    /// @todo Add description?
};

std::unordered_map<std::string, int> const kPriorityWeights = {
    {"high", 0},
    {    "", 1},
    { "low", 2}
};

// Process a single field name and update the description
void HandleField(std::string const & fieldName, editor::TypeAggregatedDescription & outDesc)
{
  if (fieldName == "name")
  {
    outDesc.m_name = true;
    return;
  }

  if (fieldName == "street" || fieldName == "housenumber" || fieldName == "housename" || fieldName == "postcode")
  {
    outDesc.m_address = true;
    return;
  }

  if (fieldName == "cuisine")
  {
    outDesc.m_cuisine = true;
    return;
  }

  auto const it = kNamesToFMD.find(fieldName);
  ASSERT(it != kNamesToFMD.end(), ("Unknown field in editor config:", fieldName));
  outDesc.m_editableFields.push_back(it->second);
}
}  // namespace

std::string EditorConfig::GetOsmTagsKey(std::vector<std::pair<std::string, std::string>> tags) const
{
  std::sort(tags.begin(), tags.end(), [](auto const & a, auto const & b) { return a.first < b.first; });

  std::ostringstream ss;
  for (size_t i = 0; i < tags.size(); ++i)
    ss << tags[i].first << "=" << tags[i].second << (i == tags.size() - 1 ? "" : ";");
  return ss.str();
}

bool EditorConfig::GetTypeDescription(std::vector<std::string> classificatorTypes,
                                      TypeAggregatedDescription & outDesc) const
{
  bool isBuilding = false;
  base::EraseIf(classificatorTypes, [&isBuilding, &outDesc](std::string const & type)
  {
    if (type == "building")
    {
      isBuilding = true;
      outDesc.m_address = true;
      outDesc.m_editableFields.push_back(EType::FMD_BUILDING_LEVELS);
      outDesc.m_editableFields.push_back(EType::FMD_POSTCODE);
      return true;
    }
    return false;
  });

  std::vector<std::string> addTypes;
  for (auto const & type : classificatorTypes)
  {
    size_t pos = 0;
    while ((pos = type.find('-', pos)) != std::string::npos)
    {
      addTypes.push_back(type.substr(0, pos));
      pos++;
    }
  }
  classificatorTypes.insert(classificatorTypes.end(), addTypes.begin(), addTypes.end());

  std::vector<std::pair<config::Type const *, size_t>> matches;
  for (size_t i = 0; i < m_root.types.size(); ++i)
    if (base::IsExist(classificatorTypes, m_root.types[i].id))
      matches.emplace_back(&m_root.types[i], i);

  if (matches.empty())
    return isBuilding;

  // Sorting multiple matches by priority, with the criteria: first by explicit priority tag,then by original file order
  std::sort(matches.begin(), matches.end(), [](auto const & a, auto const & b)
  {
    int const weightA = kPriorityWeights.count(a.first->priority.value_or(""))
                          ? kPriorityWeights.at(a.first->priority.value_or(""))
                          : 1;
    int const weightB = kPriorityWeights.count(b.first->priority.value_or(""))
                          ? kPriorityWeights.at(b.first->priority.value_or(""))
                          : 1;
    if (weightA != weightB)
      return weightA < weightB;
    return a.second < b.second;
  });

  auto const * bestMatch = matches.front().first;
  if (bestMatch->editable.value_or(true) == false)
    return isBuilding;

  // Using set to avoid duplicate fields.
  std::set<std::string> fields;
  if (bestMatch->include)
    fields.insert(bestMatch->include->begin(), bestMatch->include->end());
  if (bestMatch->include_groups)
  {
    for (auto const & groupName : *bestMatch->include_groups)
    {
      auto const it = m_root.groups.find(groupName);
      if (it != m_root.groups.end())
        fields.insert(it->second.fields.begin(), it->second.fields.end());
    }
  }

  for (auto const & fieldName : fields)
    HandleField(fieldName, outDesc);

  base::SortUnique(outDesc.m_editableFields);
  return true;
}

// REPLACE the existing GetTypesThatCanBeAdded method with this:
std::vector<std::string> EditorConfig::GetTypesThatCanBeAdded() const
{
  std::vector<std::string> result;
  for (auto const & preset : m_root.types)
    if (preset.can_add.value_or(true) && preset.editable.value_or(true))
      result.push_back(preset.id);
  return result;
}

std::vector<std::pair<std::string, std::string>> EditorConfig::GetPrimaryTags(
    std::string const & classificatorType) const
{
  std::vector<std::pair<std::string, std::string>> result;
  auto const it = std::find_if(m_root.types.begin(), m_root.types.end(),
                               [&classificatorType](config::Type const & p) { return p.id == classificatorType; });

  if (it != m_root.types.end() && it->tags)
  {
    result.reserve(it->tags->size());
    for (auto const & [key, value] : *it->tags)
      result.emplace_back(key, value);
  }
  return result;
}

std::string EditorConfig::GetOmType(std::vector<std::pair<std::string, std::string>> const & tags) const
{
  if (tags.empty())
    return {};

  auto const it = m_osmTagsToOmType.find(GetOsmTagsKey(tags));
  if (it != m_osmTagsToOmType.end())
    return it->second;

  return {};
}

void EditorConfig::BuildReverseMapping()
{
  m_osmTagsToOmType.clear();
  for (auto const & preset : m_root.types)
  {
    if (preset.tags && !preset.tags->empty())
    {
      std::vector<std::pair<std::string, std::string>> tagsVector;
      tagsVector.reserve(preset.tags->size());
      for (auto const & [key, val] : *preset.tags)
        tagsVector.emplace_back(key, val);

      m_osmTagsToOmType[GetOsmTagsKey(std::move(tagsVector))] = preset.id;
    }
  }
}

void EditorConfig::SetConfig(std::string_view jsonBuffer)
{
  auto editorJson = glz::read_json<config::Root>(jsonBuffer);
  if (editorJson)
  {
    m_root = std::move(editorJson.value());
    BuildReverseMapping();
  }
  else
  {
    LOG(LERROR, ("Failed to parse editor.json with glaze:", glz::format_error(editorJson.error(), jsonBuffer)));
  }
}
}  // namespace editor
