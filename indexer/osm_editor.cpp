#include "indexer/classificator.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/index.hpp"
#include "indexer/osm_editor.hpp"

#include "platform/platform.hpp"

#include "editor/xml_feature.hpp"

#include "base/logging.hpp"

#include "std/map.hpp"
#include "std/set.hpp"

#include "3party/pugixml/src/pugixml.hpp"

using namespace pugi;
using feature::EGeomType;
using feature::Metadata;

constexpr char const * kEditorXMLFileName = "edits.xml";
constexpr char const * kXmlRootNode = "mapsme";
constexpr char const * kDeleteSection = "delete";
constexpr char const * kModifySection = "modify";
constexpr char const * kCreateSection = "create";

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
  // TODO(mgsergio): Implement XML deserialization into m_features.
}

void Editor::Save(string const & fullFilePath) const
{
  // Should we delete edits file if user has canceled all changes?
  if (m_features.empty())
    return;

  xml_document doc;
  xml_node root = doc.append_child(kXmlRootNode);
  // Use format_version for possible future format changes.
  root.append_attribute("format_version") = 1;
  for (auto const & mwm : m_features)
  {
    xml_node mwmNode = root.append_child("mwm");
    mwmNode.append_attribute("name") = mwm.first.GetInfo()->GetCountryName().c_str();
    mwmNode.append_attribute("version") = mwm.first.GetInfo()->GetVersion();
    xml_node deleted = mwmNode.append_child(kDeleteSection);
    xml_node modified = mwmNode.append_child(kModifySection);
    xml_node created = mwmNode.append_child(kCreateSection);
    for (auto const & offset : mwm.second)
    {
      FeatureTypeInfo const & fti = offset.second;
      editor::XMLFeature xf = fti.m_feature.ToXML();
      xf.SetModificationTime(fti.m_modificationTimestamp);
      if (fti.m_uploadAttemptTimestamp)
      {
        xf.SetUploadTime(fti.m_uploadAttemptTimestamp);
        xf.SetUploadStatus(fti.m_uploadStatus);
        xf.SetUploadError(fti.m_uploadError);
      }
      switch (fti.m_status)
      {
      case EDeleted: VERIFY(xf.AttachToParentNode(deleted), ()); break;
      case EModified: VERIFY(xf.AttachToParentNode(modified), ()); break;
      case ECreated: VERIFY(xf.AttachToParentNode(created), ()); break;
      case EUntouched: CHECK(false, ("Not edited features shouldn't be here."));
      }
    }
  }

  if (doc && !doc.save_file(fullFilePath.c_str(), "  "))
    LOG(LERROR, ("Can't save map edits into", fullFilePath));
}

Editor::FeatureStatus Editor::GetFeatureStatus(MwmSet::MwmId const & mwmId, uint32_t offset) const
{
  // Most popular case optimization.
  if (m_features.empty())
    return EUntouched;

  auto const mwmMatched = m_features.find(mwmId);
  if (mwmMatched == m_features.end())
    return EUntouched;

  auto const offsetMatched = mwmMatched->second.find(offset);
  if (offsetMatched == mwmMatched->second.end())
    return EUntouched;

  return offsetMatched->second.m_status;
}

void Editor::DeleteFeature(FeatureType const & feature)
{
  FeatureID const fid = feature.GetID();
  FeatureTypeInfo & ftInfo = m_features[fid.m_mwmId][fid.m_index];
  ftInfo.m_status = EDeleted;
  ftInfo.m_feature = feature;
  // TODO: What if local client time is absolutely wrong?
  ftInfo.m_modificationTimestamp = time(nullptr);

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
  // TODO(AlexZ): Check if feature has not changed and reset status.
  FeatureID const fid = editedFeature.GetID();
  FeatureTypeInfo & ftInfo = m_features[fid.m_mwmId][fid.m_index];
  ftInfo.m_status = EModified;
  ftInfo.m_feature = editedFeature;
  // TODO: What if local client time is absolutely wrong?
  ftInfo.m_modificationTimestamp = time(nullptr);

  // TODO(AlexZ): Synchronize Save call/make it on a separate thread.
  Save(GetEditorFilePath());

  if (m_invalidateFn)
    m_invalidateFn();
}

void Editor::ForEachFeatureInMwmRectAndScale(MwmSet::MwmId const & id,
                                             TFeatureIDFunctor const & f,
                                             m2::RectD const & rect,
                                             uint32_t /*scale*/)
{
  auto const mwmFound = m_features.find(id);
  if (mwmFound == m_features.end())
    return;

  // TODO(AlexZ): Check that features are visible at this scale.
  // Process only new (created) features.
  for (auto const & offset : mwmFound->second)
  {
    FeatureTypeInfo const & ftInfo = offset.second;
    if (ftInfo.m_status == ECreated && rect.IsPointInside(ftInfo.m_feature.GetCenter()))
      f(FeatureID(id, offset.first));
  }
}

void Editor::ForEachFeatureInMwmRectAndScale(MwmSet::MwmId const & id,
                                             TFeatureTypeFunctor const & f,
                                             m2::RectD const & rect,
                                             uint32_t /*scale*/)
{
  auto mwmFound = m_features.find(id);
  if (mwmFound == m_features.end())
    return;

  // TODO(AlexZ): Check that features are visible at this scale.
  // Process only new (created) features.
  for (auto & offset : mwmFound->second)
  {
    FeatureTypeInfo & ftInfo = offset.second;
    if (ftInfo.m_status == ECreated && rect.IsPointInside(ftInfo.m_feature.GetCenter()))
      f(ftInfo.m_feature);
  }
}

bool Editor::GetEditedFeature(MwmSet::MwmId const & mwmId, uint32_t offset, FeatureType & outFeature) const
{
  auto const mwmMatched = m_features.find(mwmId);
  if (mwmMatched == m_features.end())
    return false;

  auto const offsetMatched = mwmMatched->second.find(offset);
  if (offsetMatched == mwmMatched->second.end())
    return false;

  // TODO(AlexZ): Should we process deleted/created features as well?
  outFeature = offsetMatched->second.m_feature;
  return true;
}

vector<Metadata::EType> Editor::EditableMetadataForType(uint32_t type) const
{
  // TODO(mgsergio): Load editable fields into memory from XML and query them here.
  // Enable opening hours for the first release.
  return {Metadata::FMD_OPEN_HOURS};
}

}  // namespace osm
