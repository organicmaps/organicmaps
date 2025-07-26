#pragma once

#include "cppjansson/cppjansson.hpp"
#include "indexer/feature_meta.hpp"

#include <string>
#include <vector>

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

  void SetConfig(base::Json const & doc);

  // TODO(mgsergio): Implement this getter to avoid hard-code in XMLFeature::ApplyPatch.
  // It should return [[phone, contact:phone, contact:mobile], [website, contact:website, url], ...].
  // vector<vector<string>> GetAlternativeFields() const;
  std::vector<std::pair<std::string, std::string>> GetPrimaryTags(std::string const & classificatorType) const;
  std::string GetOmType(std::vector<std::pair<std::string, std::string>> const & tags) const;

private:
  std::string GetOsmTagsKey(std::vector<std::pair<std::string, std::string>> tags) const;
  void BuildReverseMapping();

  base::Json m_document;
  std::map<std::string, std::string> m_osmTagsToOmType;
};
}  // namespace editor