#pragma once

#include "indexer/feature_meta.hpp"

#include <string>
#include <vector>

#include <pugixml.hpp>

class Reader;

namespace editor
{
struct TypeAggregatedDescription
{
  using EType = feature::Metadata::EType;
  using FeatureFields = std::vector<EType>;

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

  void SetConfig(pugi::xml_document const & doc);

private:
  pugi::xml_document m_document;
};
}  // namespace editor
