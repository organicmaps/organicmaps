#include "editor/editor_config.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <string>
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
    {"ele", EType::FMD_ELE},
    // {"", EType::FMD_TURN_LANES},
    // {"", EType::FMD_TURN_LANES_FORWARD},
    // {"", EType::FMD_TURN_LANES_BACKWARD},
    {"email", EType::FMD_EMAIL},
    {"postcode", EType::FMD_POSTCODE},
    {"wikipedia", EType::FMD_WIKIPEDIA},
    // {"", EType::FMD_MAXSPEED},
    {"flats", EType::FMD_FLATS},
    {"height", EType::FMD_HEIGHT},
    // {"", EType::FMD_MIN_HEIGHT},
    {"denomination", EType::FMD_DENOMINATION},
    {"building:levels", EType::FMD_BUILDING_LEVELS},
    {"level", EType::FMD_LEVEL}
    /// @todo Add description?
};

std::unordered_map<std::string, int> const kPriorityWeights = {{"high", 0}, {"", 1}, {"low", 2}};

bool TypeDescriptionFromXml(pugi::xml_node const & root, pugi::xml_node const & node,
                            editor::TypeAggregatedDescription & outDesc)
{
  if (!node || strcmp(node.attribute("editable").value(), "no") == 0)
    return false;

  auto const handleField = [&outDesc](std::string const & fieldName) {
    if (fieldName == "name")
    {
      outDesc.m_name = true;
      return;
    }

    if (fieldName == "street" || fieldName == "housenumber" || fieldName == "housename" ||
        fieldName == "postcode")
    {
      outDesc.m_address = true;
      return;
    }

    if (fieldName == "cuisine")
    {
      outDesc.m_cuisine = true;
      return;
    }

    // TODO(mgsergio): Add support for non-metadata fields like atm, wheelchair, toilet etc.
    auto const it = kNamesToFMD.find(fieldName);

    ASSERT(it != end(kNamesToFMD), ("Wrong field:", fieldName));
    outDesc.m_editableFields.push_back(it->second);
  };

  for (auto const & xNode : node.select_nodes("include[@group]"))
  {
    auto const node = xNode.node();
    std::string const groupName = node.attribute("group").value();

    std::string const xpath = "/mapsme/editor/fields/field_group[@name='" + groupName + "']";
    auto const group = root.select_node(xpath.data()).node();
    ASSERT(group, ("No such group", groupName));

    for (auto const & fieldRefXName : group.select_nodes("field_ref/@name"))
    {
      auto const fieldName = fieldRefXName.attribute().value();
      handleField(fieldName);
    }
  }

  for (auto const & xNode : node.select_nodes("include[@field]"))
  {
    auto const node = xNode.node();
    std::string const fieldName = node.attribute("field").value();
    handleField(fieldName);
  }

  // Ordered by Metadata::EType value, which is also satisfy fields importance.
  base::SortUnique(outDesc.m_editableFields);
  return true;
}

/// The priority is defined by elems order, except elements with priority="high".
std::vector<pugi::xml_node> GetPrioritizedTypes(pugi::xml_node const & node)
{
  std::vector<pugi::xml_node> result;
  for (auto const & xNode : node.select_nodes("/mapsme/editor/types/type[@id]"))
    result.push_back(xNode.node());
  stable_sort(begin(result), end(result),
              [](pugi::xml_node const & lhs, pugi::xml_node const & rhs) {
                auto const lhsWeight = kPriorityWeights.find(lhs.attribute("priority").value());
                auto const rhsWeight = kPriorityWeights.find(rhs.attribute("priority").value());

                CHECK(lhsWeight != kPriorityWeights.end(), (""));
                CHECK(rhsWeight != kPriorityWeights.end(), (""));

                return lhsWeight->second < rhsWeight->second;
              });
  return result;
}
}  // namespace

bool EditorConfig::GetTypeDescription(std::vector<std::string> classificatorTypes,
                                      TypeAggregatedDescription & outDesc) const
{
  bool isBuilding = false;
  std::vector<std::string> addTypes;
  for (auto it = classificatorTypes.begin(); it != classificatorTypes.end(); ++it)
  {
    if (*it == "building")
    {
      outDesc.m_address = isBuilding = true;
      outDesc.m_editableFields.push_back(EType::FMD_BUILDING_LEVELS);
      outDesc.m_editableFields.push_back(EType::FMD_POSTCODE);
      classificatorTypes.erase(it);
      break;
    }

    // Adding partial types for 2..N-1 parts of a N-part type.
    auto hyphenPos = it->find('-');
    while ((hyphenPos = it->find('-', hyphenPos + 1)) != std::string::npos)
      addTypes.push_back(it->substr(0, hyphenPos));
  }

  classificatorTypes.insert(classificatorTypes.end(), addTypes.begin(), addTypes.end());

  auto const typeNodes = GetPrioritizedTypes(m_document);
  auto const it = base::FindIf(typeNodes, [&classificatorTypes](pugi::xml_node const & node)
  {
    return base::IsExist(classificatorTypes, node.attribute("id").value());
  });
  if (it == end(typeNodes))
    return isBuilding;

  return TypeDescriptionFromXml(m_document, *it, outDesc);
}

std::vector<std::string> EditorConfig::GetTypesThatCanBeAdded() const
{
  auto const xpathResult =
      m_document.select_nodes("/mapsme/editor/types/type[not(@can_add='no' or @editable='no')]");

  std::vector<std::string> result;
  for (auto const & xNode : xpathResult)
    result.emplace_back(xNode.node().attribute("id").value());
  return result;
}

void EditorConfig::SetConfig(pugi::xml_document const & doc) { m_document.reset(doc); }
}  // namespace editor
