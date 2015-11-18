#include "indexer/classificator.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/index.hpp"
#include "indexer/osm_editor.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "std/map.hpp"
#include "std/set.hpp"

#include "3party/pugixml/src/pugixml.hpp"

using namespace pugi;
using feature::EGeomType;
using feature::Metadata;

static char constexpr const * kEditorXMLFileName = "edits.xml";

namespace osm
{

// TODO(AlexZ): Normalize osm multivalue strings for correct merging
// (e.g. insert/remove spaces after ';' delimeter);

namespace
{
string GetEditorFilePath() { return GetPlatform().WritablePathForFile(kEditorXMLFileName); }
} // namespace

Editor::Editor()
{
  // Load all previous edits from persistent storage.
  Load(GetEditorFilePath());
}

Editor & Editor::Instance()
{
  static Editor instance;
  return instance;
}

void Editor::Load(string const & fullFilePath)
{
  xml_document xml;
  xml_parse_result res = xml.load_file(fullFilePath.c_str());
  // Note: status_file_not_found is ok if user has never made any edits.
  if (res != status_ok && res != status_file_not_found)
  {
    LOG(LERROR, ("Can't load XML Edits from disk:", fullFilePath));
  }
  // TODO(mgsergio): Implement XML deserialization into m_[deleted|edited|created]Features.
}

void Editor::Save(string const & /*fullFilePath*/) const
{
  // Do not save empty xml file if no any edits were made.
  if (m_deletedFeatures.empty() && m_editedFeatures.empty() && m_createdFeatures.empty())
    return;
  // TODO(mgsergio): Implement XML serialization from m_[deleted|edited|created]Features.
}

bool Editor::IsFeatureDeleted(FeatureID const & fid) const
{
  // Most popular case optimization.
  if (m_deletedFeatures.empty())
    return false;

  return m_deletedFeatures.find(fid) != m_deletedFeatures.end();
}

void Editor::DeleteFeature(FeatureType const & feature)
{
  m_deletedFeatures.insert(feature.GetID());
  // TODO(AlexZ): Synchronize Save call/make it on a separate thread.
  Save(GetEditorFilePath());

  if (m_invalidateFn)
    m_invalidateFn();
}

//namespace
//{
//FeatureID GenerateNewFeatureId(FeatureID const & oldFeatureId)
//{
//  // TODO(AlexZ): Stable & unique features ID generation.
//  static uint32_t newOffset = 0x0effffff;
//  return FeatureID(oldFeatureId.m_mwmId, newOffset++);
//}
//}  // namespace

void Editor::EditFeature(FeatureType & editedFeature)
{
  m_editedFeatures[editedFeature.GetID()] = editedFeature;
  // TODO(AlexZ): Synchronize Save call/make it on a separate thread.
  Save(GetEditorFilePath());

  if (m_invalidateFn)
    m_invalidateFn();
}

bool Editor::IsFeatureEdited(FeatureID const & fid) const
{
  return m_editedFeatures.find(fid) != m_editedFeatures.end();
}

void Editor::ForEachFeatureInMwmRectAndScale(MwmSet::MwmId const & id,
                                             function<void(FeatureID const &)> const & f,
                                             m2::RectD const & /*rect*/,
                                             uint32_t /*scale*/)
{
  // TODO(AlexZ): Check that features are in the rect and are visible at this scale.
  for (auto & feature : m_createdFeatures)
  {
    if (feature.first.m_mwmId == id)
      f(feature.first);
  }
}

void Editor::ForEachFeatureInMwmRectAndScale(MwmSet::MwmId const & id,
                                             function<void(FeatureType &)> const & f,
                                             m2::RectD const & /*rect*/,
                                             uint32_t /*scale*/)
{
  // TODO(AlexZ): Check that features are in the rect and are visible at this scale.
  for (auto & feature : m_createdFeatures)
  {
    if (feature.first.m_mwmId == id)
      f(feature.second);
  }
}

bool Editor::GetEditedFeature(FeatureID const & fid, FeatureType & outFeature) const
{
  auto found = m_editedFeatures.find(fid);
  if (found == m_editedFeatures.end())
    return false;
  outFeature = found->second;
  return true;
}

vector<Metadata::EType> Editor::EditableMetadataForType(uint32_t type) const
{
  // TODO(mgsergio): Load editable fields into memory from XML and query them here.
  // Enable opening hours for the first release.
  return {Metadata::FMD_OPEN_HOURS};
}

}  // namespace osm
