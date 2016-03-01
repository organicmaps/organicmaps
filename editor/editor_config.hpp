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
class TypeAggregatedDescription
{
public:
  using EType = feature::Metadata::EType;
  using TFeatureFields = set<EType>;

  TypeAggregatedDescription(TFeatureFields const & editableFields,
                            bool const name, bool const address)
      : m_editableFields(editableFields),
        m_name(name),
        m_address(address)
  {
  }

  TypeAggregatedDescription()
      : m_editableFields(),
        m_name(false),
        m_address(false)
  {
  }

  bool IsEmpty() const
  {
    return IsNameEditable() || IsAddressEditable() || !m_editableFields.empty();
  }

  TFeatureFields const & GetEditableFields() const { return m_editableFields; }

  bool IsNameEditable() const { return m_name; };
  bool IsAddressEditable() const { return m_address; }

private:
  TFeatureFields m_editableFields;

  bool m_name;
  bool m_address;
};

DECLARE_EXCEPTION(ConfigLoadError, RootException);

class EditorConfig
{
public:
  EditorConfig(string const & fileName);

  TypeAggregatedDescription GetTypeDescription(vector<string> const & classificatorTypes) const;
  vector<string> GetTypesThatCanBeAdded() const;

  bool EditingEnable() const;

  void Reload();

private:
  string const m_fileName;
  pugi::xml_document m_document;
};
}  // namespace editor
