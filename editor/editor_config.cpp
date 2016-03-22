#include "editor/editor_config.hpp"

#include "platform/platform.hpp"

#include "coding/reader.hpp"

#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"
#include "std/cstring.hpp"
#include "std/unordered_map.hpp"

namespace
{
using EType = feature::Metadata::EType;

// TODO(mgsergio): It would be nice to have this map generated from editor.config.
static unordered_map<string, EType> const kNamesToFMD= {
  {"cuisine", feature::Metadata::FMD_CUISINE},
  {"opening_hours", feature::Metadata::FMD_OPEN_HOURS},
  {"phone", feature::Metadata::FMD_PHONE_NUMBER},
  {"fax", feature::Metadata::FMD_FAX_NUMBER},
  {"stars", feature::Metadata::FMD_STARS},
  {"operator", feature::Metadata::FMD_OPERATOR},
  // {"", feature::Metadata::FMD_URL},
  {"website", feature::Metadata::FMD_WEBSITE},
  {"internet", feature::Metadata::FMD_INTERNET},
  {"ele", feature::Metadata::FMD_ELE},
  // {"", feature::Metadata::FMD_TURN_LANES},
  // {"", feature::Metadata::FMD_TURN_LANES_FORWARD},
  // {"", feature::Metadata::FMD_TURN_LANES_BACKWARD},
  {"email", feature::Metadata::FMD_EMAIL},
  {"postcode", feature::Metadata::FMD_POSTCODE},
  {"wikipedia", feature::Metadata::FMD_WIKIPEDIA},
  // {"", feature::Metadata::FMD_MAXSPEED},
  {"flats", feature::Metadata::FMD_FLATS},
  {"height", feature::Metadata::FMD_HEIGHT},
  // {"", feature::Metadata::FMD_MIN_HEIGHT},
  {"denomination", feature::Metadata::FMD_DENOMINATION},
  {"building_levels", feature::Metadata::FMD_BUILDING_LEVELS}
  // description
};

bool TypeDescriptionFromXml(pugi::xml_node const & root, pugi::xml_node const & node,
                            editor::TypeAggregatedDescription & outDesc)
{
  if (!node || strcmp(node.attribute("editable").value(), "no") == 0)
    return false;

  auto const handleField = [&outDesc](string const & fieldName)
  {
    if (fieldName == "name")
    {
      outDesc.m_name = true;
      return;
    }

    if (fieldName == "street" || fieldName == "housenumber" || fieldName == "housename")
    {
      outDesc.m_address = true;
      return;
    }

    // TODO(mgsergio): Add support for non-metadata fields like atm, wheelchair, toilet etc.
    auto const it = kNamesToFMD.find(fieldName);
    ASSERT(it != end(kNamesToFMD), ("Wrong field:", fieldName));
    outDesc.m_editableFields.push_back(it->second);
  };

  for (auto const xNode : node.select_nodes("include[@group]"))
  {
    auto const node = xNode.node();
    string const groupName = node.attribute("group").value();

    string const xpath = "/mapsme/editor/fields/field_group[@name='" + groupName + "']";
    auto const group = root.select_node(xpath.data()).node();
    ASSERT(group, ("No such group", groupName));

    for (auto const fieldRefXName : group.select_nodes("field_ref/@name"))
    {
      auto const fieldName = fieldRefXName.attribute().value();
      handleField(fieldName);
    }
  }

  for (auto const xNode : node.select_nodes("include[@field]"))
  {
    auto const node = xNode.node();
    string const fieldName = node.attribute("field").value();
      handleField(fieldName);
  }

  my::SortUnique(outDesc.m_editableFields);
  return true;
}

/// The priority is defined by elems order, except elements with priority="high".
vector<pugi::xml_node> GetPrioritizedTypes(pugi::xml_node const & node)
{
  vector<pugi::xml_node> result;
  for (auto const xNode : node.select_nodes("/mapsme/editor/types/type[@id]"))
    result.push_back(xNode.node());
  stable_sort(begin(result), end(result),
              [](pugi::xml_node const & a, pugi::xml_node const & b)
              {
                if (strcmp(a.attribute("priority").value(), "high") != 0 &&
                    strcmp(b.attribute("priority").value(), "high") == 0)
                  return true;
                return false;
              });
  return result;
}
}  // namespace

namespace editor
{
EditorConfig::EditorConfig(string const & fileName)
    : m_fileName(fileName)
{
  Reload();
}

bool EditorConfig::GetTypeDescription(vector<string> const & classificatorTypes,
                                      TypeAggregatedDescription & outDesc) const
{
  auto const typeNodes = GetPrioritizedTypes(m_document);
  auto const it = find_if(begin(typeNodes), end(typeNodes),
                          [&classificatorTypes](pugi::xml_node const & node)
                          {
                            return find(begin(classificatorTypes), end(classificatorTypes),
                                        node.attribute("id").value()) != end(classificatorTypes);
                          });
  if (it == end(typeNodes))
    return false;

  return TypeDescriptionFromXml(m_document, *it, outDesc);
}

vector<string> EditorConfig::GetTypesThatCanBeAdded() const
{
  auto const xpathResult = m_document.select_nodes("/mapsme/editor/types/type[not(@can_add='no' or @editable='no')]");
  vector<string> result;
  for (auto const xNode : xpathResult)
    result.emplace_back(xNode.node().attribute("id").value());
  return result;
}

void EditorConfig::Reload()
{
  string content;
  auto const reader = GetPlatform().GetReader(m_fileName);
  reader->ReadAsString(content);
  if (!m_document.load_buffer(content.data(), content.size()))
    MYTHROW(ConfigLoadError, ("Can't parse config"));
}
}  // namespace editor
