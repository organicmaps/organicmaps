#pragma once

#include <map>
#include <optional>
#include "3party/glaze/include/glaze/glaze.hpp"
#include "indexer/feature_meta.hpp"

#include <string>
#include <vector>

namespace editor::config
{
struct Field
{
  std::optional<bool> editable;
  std::optional<bool> multilanguage;
  std::optional<std::vector<std::string>> tags;
  std::optional<std::string> value_type;
  std::optional<bool> allow_multiple;
  std::optional<std::vector<std::string>> options;
};

struct Group
{
  std::vector<std::string> fields;
};

struct Type
{
  std::string id;
  std::optional<bool> can_add;
  std::optional<bool> editable;
  std::optional<std::string> group;
  std::optional<std::string> priority;
  std::optional<std::vector<std::string>> include;
  std::optional<std::vector<std::string>> include_groups;
  std::optional<std::map<std::string, std::string>> tags;
};

struct Root
{
  std::map<std::string, Field> fields;
  std::map<std::string, Group> groups;
  std::vector<Type> types;
};
}  // namespace editor::config

template <>
struct glz::meta<editor::config::Field>
{
  using T = editor::config::Field;
  static constexpr auto value =
      object("editable", &T::editable, "multilanguage", &T::multilanguage, "tags", &T::tags, "value_type",
             &T::value_type, "allow_multiple", &T::allow_multiple, "options", &T::options);
};
template <>
struct glz::meta<editor::config::Group>
{
  using T = editor::config::Group;
  static constexpr auto value = object("fields", &T::fields);
};
template <>
struct glz::meta<editor::config::Type>
{
  using T = editor::config::Type;
  static constexpr auto value =
      object("id", &T::id, "can_add", &T::can_add, "editable", &T::editable, "group", &T::group, "priority",
             &T::priority, "include", &T::include, "include_groups", &T::include_groups, "tags", &T::tags);
};
template <>
struct glz::meta<editor::config::Root>
{
  using T = editor::config::Root;
  static constexpr auto value = object("fields", &T::fields, "groups", &T::groups, "types", &T::types);
};

namespace editor
{
struct TypeAggregatedDescription
{
  using EType = feature::Metadata::EType;
  using FeatureFields = std::vector<EType>;

  bool IsEmpty() const
  {
    return IsNameEditable() || IsAddressEditable() || IsCuisineEditable() || !m_editableFields.empty();
  }

  FeatureFields const & GetEditableFields() const { return m_editableFields; }

  bool IsNameEditable() const { return m_name; }
  bool IsAddressEditable() const { return m_address; }
  bool IsCuisineEditable() const { return m_cuisine; }

  FeatureFields m_editableFields;

  bool m_name = false;
  bool m_address = false;
  bool m_cuisine = false;
};

class EditorConfig
{
public:
  EditorConfig() = default;

  // TODO(mgsergio): Reduce overhead by matching uint32_t types instead of strings.
  bool GetTypeDescription(std::vector<std::string> classificatorTypes, TypeAggregatedDescription & outDesc) const;
  std::vector<std::string> GetTypesThatCanBeAdded() const;

  void SetConfig(std::string_view jsonBuffer);

  // TODO(mgsergio): Implement this getter to avoid hard-code in XMLFeature::ApplyPatch.
  // It should return [[phone, contact:phone, contact:mobile], [website, contact:website, url], ...].
  // vector<vector<string>> GetAlternativeFields() const;
  std::vector<std::pair<std::string, std::string>> GetPrimaryTags(std::string const & classificatorType) const;
  std::string GetOmType(std::vector<std::pair<std::string, std::string>> const & tags) const;

private:
  std::string GetOsmTagsKey(std::vector<std::pair<std::string, std::string>> tags) const;
  void BuildReverseMapping();

  config::Root m_root;
  std::map<std::string, std::string> m_osmTagsToOmType;
};
}  // namespace editor