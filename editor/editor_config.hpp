#pragma once

#include "indexer/feature_meta.hpp"

#include "base/exception.hpp"

#include "std/set.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

#include "3party/pugixml/src/pugixml.hpp"

class Reader;

namespace editor
{
struct TypeAggregatedDescription
{
  using EType = feature::Metadata::EType;
  using TFeatureFields = vector<EType>;

  bool IsEmpty() const
  {
    return IsNameEditable() || IsAddressEditable() || !m_editableFields.empty();
  }

  TFeatureFields const & GetEditableFields() const { return m_editableFields; }

  bool IsNameEditable() const { return m_name; }
  bool IsAddressEditable() const { return m_address; }

  TFeatureFields m_editableFields;

  bool m_name = false;
  bool m_address = false;
};

DECLARE_EXCEPTION(ConfigLoadError, RootException);

class EditorConfig
{
public:
  EditorConfig(string const & fileName = "editor.config");

  bool GetTypeDescription(vector<string> const & classificatorTypes, TypeAggregatedDescription & outDesc) const;
  vector<string> GetTypesThatCanBeAdded() const;

  bool EditingEnable() const;

  void Reload();

  // TODO(mgsergio): Implement this getter to avoid hard-code in XMLFeature::ApplyPatch.
  // It should return [[phone, contact:phone], [website, contact:website, url], ...].
  //vector<vector<string>> GetAlternativeFields() const;

private:
  string const m_fileName;
  pugi::xml_document m_document;
};
}  // namespace editor
