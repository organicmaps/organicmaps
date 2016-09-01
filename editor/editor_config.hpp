#pragma once

#include "indexer/feature_meta.hpp"

#include "std/mutex.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
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

class EditorConfig
{
public:
  EditorConfig() = default;

  // TODO(mgsergio): Reduce overhead by matching uint32_t types instead of strings.
  bool GetTypeDescription(vector<string> classificatorTypes,
                          TypeAggregatedDescription & outDesc) const;
  vector<string> GetTypesThatCanBeAdded() const;

  void SetConfig(pugi::xml_document const & doc);

  // TODO(mgsergio): Implement this getter to avoid hard-code in XMLFeature::ApplyPatch.
  // It should return [[phone, contact:phone], [website, contact:website, url], ...].
  //vector<vector<string>> GetAlternativeFields() const;

private:
  pugi::xml_document m_document;
};

// Class which provides methods for EditorConfig concurrently using.
class EditorConfigWrapper
{
public:
  EditorConfigWrapper() = default;

  void Set(shared_ptr<EditorConfig> config)
  {
    lock_guard<mutex> lock(m_mu);
    m_config = config;
  }

  shared_ptr<EditorConfig const> Get() const
  {
    lock_guard<mutex> lock(m_mu);
    return m_config;
  }

private:
  // It's possible to use atomic_{load|store} here instead of mutex,
  // but seems that libstdc++4.9 doesn't support it. Need to rewrite
  // this code as soon as libstdc++5 will be ready for lastest Debian
  // release, or as soon as atomic_shared_ptr will be ready.
  mutable mutex m_mu;
  shared_ptr<EditorConfig> m_config = make_shared<EditorConfig>();

  // Just in case someone tryes to pass EditorConfigWrapper by value instead of referense.
  DISALLOW_COPY_AND_MOVE(EditorConfigWrapper);
};
}  // namespace editor
