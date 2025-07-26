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

bool TypeDescriptionFromJson(json_t const * rootDoc, json_t const * typeObject,
                             editor::TypeAggregatedDescription & outDesc)
{
  if (!typeObject)
    return false;

  // Check if marked as non-editable.
  auto const * editableNode = json_object_get(typeObject, "editable");
  if (editableNode && json_is_false(editableNode))
    return false;

  // Using set to avoid duplicate fields.
  std::set<std::string> fields;

  // Include fields directly listed
  auto const * includeNode = json_object_get(typeObject, "include");
  if (includeNode && json_is_array(includeNode))
  {
    for (size_t i = 0; i < json_array_size(includeNode); ++i)
    {
      auto const * fieldNode = json_array_get(includeNode, i);
      if (fieldNode && json_is_string(fieldNode))
        fields.insert(json_string_value(fieldNode));
    }
  }

  // "include_groups" fields
  auto const * groupsNode = json_object_get(rootDoc, "groups");
  auto const * includeGroupsNode = json_object_get(typeObject, "include_groups");

  if (groupsNode && json_is_object(groupsNode) && includeGroupsNode && json_is_array(includeGroupsNode))
  {
    for (size_t i = 0; i < json_array_size(includeGroupsNode); ++i)
    {
      auto const * groupNameNode = json_array_get(includeGroupsNode, i);
      if (groupNameNode && json_is_string(groupNameNode))
      {
        auto const * groupObj = json_object_get(groupsNode, json_string_value(groupNameNode));
        if (groupObj && json_is_object(groupObj))
        {
          auto const * groupFieldsNode = json_object_get(groupObj, "fields");
          if (groupFieldsNode && json_is_array(groupFieldsNode))
          {
            for (size_t j = 0; j < json_array_size(groupFieldsNode); ++j)
            {
              auto const * fieldNode = json_array_get(groupFieldsNode, j);
              if (fieldNode && json_is_string(fieldNode))
                fields.insert(json_string_value(fieldNode));
            }
          }
        }
      }
    }
  }

  // Process all collected fields.
  for (auto const & field : fields)
    HandleField(field, outDesc);

  base::SortUnique(outDesc.m_editableFields);
  return true;
}

json_t const * GetObject(json_t const * parent, char const * key)
{
  if (!parent || !json_is_object(parent))
    return nullptr;
  auto const * obj = json_object_get(parent, key);
  return (obj && json_is_object(obj)) ? obj : nullptr;
}

std::string GetString(json_t const * parent, char const * key, std::string const & defaultValue = "")
{
  if (!parent || !json_is_object(parent))
    return defaultValue;
  auto const * node = json_object_get(parent, key);
  return (node && json_is_string(node)) ? json_string_value(node) : defaultValue;
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
  if (!m_document.get())
    return false;

  auto const * typesConfig = json_object_get(m_document.get(), "types");
  if (!typesConfig || !json_is_array(typesConfig))
    return false;

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

  std::vector<std::pair<json_t const *, size_t>> matches;
  for (size_t i = 0; i < json_array_size(typesConfig); ++i)
  {
    auto const * typeObject = json_array_get(typesConfig, i);
    std::string const id = GetString(typeObject, "id");
    if (base::IsExist(classificatorTypes, id))
      matches.emplace_back(typeObject, i);
  }

  if (matches.empty())
    return isBuilding;

  // Sorting multiple matches by priority, with the criteria: first by explicit priority tag,then by original file order
  std::sort(matches.begin(), matches.end(),
            [](std::pair<json_t const *, size_t> const & a, std::pair<json_t const *, size_t> const & b)
  {
    auto const priorityA = GetString(a.first, "priority", "");
    auto const priorityB = GetString(b.first, "priority", "");
    int const weightA = kPriorityWeights.count(priorityA) ? kPriorityWeights.at(priorityA) : 1;
    int const weightB = kPriorityWeights.count(priorityB) ? kPriorityWeights.at(priorityB) : 1;
    if (weightA != weightB)
      return weightA < weightB;
    return a.second < b.second;
  });

  auto const * bestTypeObject = matches.front().first;
  return TypeDescriptionFromJson(m_document.get(), bestTypeObject, outDesc);
}

std::vector<std::string> EditorConfig::GetTypesThatCanBeAdded() const
{
  std::vector<std::string> result;
  if (!m_document.get())
    return result;

  auto const * typesConfig = json_object_get(m_document.get(), "types");
  if (!typesConfig || !json_is_array(typesConfig))
    return result;

  for (size_t i = 0; i < json_array_size(typesConfig); ++i)
  {
    auto const * typeObject = json_array_get(typesConfig, i);
    if (!typeObject || !json_is_object(typeObject))
      continue;

    bool canAdd = true;
    auto const * canAddNode = json_object_get(typeObject, "can_add");
    if (canAddNode && json_is_false(canAddNode))
      canAdd = false;

    bool editable = true;
    auto const * editableNode = json_object_get(typeObject, "editable");
    if (editableNode && json_is_false(editableNode))
      editable = false;

    if (canAdd && editable)
      result.push_back(GetString(typeObject, "id"));
  }
  return result;
}

std::vector<std::pair<std::string, std::string>> EditorConfig::GetPrimaryTags(
    std::string const & classificatorType) const
{
  std::vector<std::pair<std::string, std::string>> result;
  if (!m_document.get() || classificatorType.empty())
    return result;

  auto const * types = json_object_get(m_document.get(), "types");
  if (!types || !json_is_array(types))
    return result;

  for (size_t i = 0; i < json_array_size(types); ++i)
  {
    auto const * typeObject = json_array_get(types, i);
    if (GetString(typeObject, "id") == classificatorType)
    {
      auto const * tagsObject = GetObject(typeObject, "tags");
      if (tagsObject)
      {
        void * iter = json_object_iter(const_cast<json_t *>(tagsObject));
        while (iter)
        {
          std::string const key = json_object_iter_key(iter);
          json_t * valueNode = json_object_iter_value(iter);
          if (json_is_string(valueNode))
            result.emplace_back(key, json_string_value(valueNode));

          iter = json_object_iter_next(const_cast<json_t *>(tagsObject), iter);
        }
      }
      break;
    }
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
  if (!m_document.get())
    return;

  auto const * types = json_object_get(m_document.get(), "types");
  if (!types || !json_is_array(types))
    return;

  for (size_t i = 0; i < json_array_size(types); ++i)
  {
    auto const * typeObject = json_array_get(types, i);
    std::string const omType = GetString(typeObject, "id");
    auto const tags = GetPrimaryTags(omType);
    if (!tags.empty())
      m_osmTagsToOmType[GetOsmTagsKey(tags)] = omType;
  }
}

void EditorConfig::SetConfig(base::Json const & doc)
{
  m_document = doc;
  BuildReverseMapping();
}
}  // namespace editor
