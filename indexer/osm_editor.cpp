#include "indexer/osm_editor.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/edits_migration.hpp"
#include "indexer/fake_feature_ids.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/index_helpers.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "editor/changeset_wrapper.hpp"
#include "editor/osm_auth.hpp"
#include "editor/server_api.hpp"
#include "editor/xml_feature.hpp"

#include "coding/internal/file_data.hpp"

#include "geometry/algorithm.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"
#include "base/thread_checker.hpp"
#include "base/timer.hpp"

#include "std/algorithm.hpp"
#include "std/chrono.hpp"
#include "std/future.hpp"
#include "std/target_os.hpp"
#include "std/tuple.hpp"
#include "std/unordered_map.hpp"
#include "std/unordered_set.hpp"

#include "3party/Alohalytics/src/alohalytics.h"
#include "3party/opening_hours/opening_hours.hpp"
#include "3party/pugixml/src/pugixml.hpp"

using namespace pugi;
using feature::EGeomType;
using feature::Metadata;
using editor::XMLFeature;

namespace
{
constexpr char const * kXmlRootNode = "mapsme";
constexpr char const * kXmlMwmNode = "mwm";
constexpr char const * kDeleteSection = "delete";
constexpr char const * kModifySection = "modify";
constexpr char const * kCreateSection = "create";
constexpr char const * kObsoleteSection = "obsolete";
/// We store edited streets in OSM-compatible way.
constexpr char const * kAddrStreetTag = "addr:street";

constexpr char const * kUploaded = "Uploaded";
constexpr char const * kDeletedFromOSMServer = "Deleted from OSM by someone";
constexpr char const * kRelationsAreNotSupported = "Relations are not supported yet";
constexpr char const * kNeedsRetry = "Needs Retry";
constexpr char const * kWrongMatch = "Matched feature has no tags";

bool NeedsUpload(string const & uploadStatus)
{
  return uploadStatus != kUploaded &&
      uploadStatus != kDeletedFromOSMServer &&
      // TODO: Remove this line when relations are supported.
      uploadStatus != kRelationsAreNotSupported &&
      // TODO: Remove this when we have better matching algorithm.
      uploadStatus != kWrongMatch;
}

/// Compares editable fields connected with feature ignoring street.
bool AreFeaturesEqualButStreet(FeatureType const & a, FeatureType const & b)
{
  feature::TypesHolder const aTypes(a);
  feature::TypesHolder const bTypes(b);

  if (!aTypes.Equals(bTypes))
    return false;

  if (a.GetHouseNumber() != b.GetHouseNumber())
    return false;

  if (!a.GetMetadata().Equals(b.GetMetadata()))
      return false;

  if (a.GetNames() != b.GetNames())
    return false;

  return true;
}

XMLFeature GetMatchingFeatureFromOSM(osm::ChangesetWrapper & cw, FeatureType const & ft)
{
  ASSERT_NOT_EQUAL(ft.GetFeatureType(), feature::GEOM_LINE,
                   ("Line features are not supported yet."));
  if (ft.GetFeatureType() == feature::GEOM_POINT)
    return cw.GetMatchingNodeFeatureFromOSM(ft.GetCenter());

  // Warning: geometry is cached in FeatureType. So if it wasn't BEST_GEOMETRY,
  // we can never have it. Features here came from editors loader and should
  // have BEST_GEOMETRY geometry.
  auto geometry = ft.GetTriangesAsPoints(FeatureType::BEST_GEOMETRY);

  // Filters out duplicate points for closed ways or triangles' vertices.
  my::SortUnique(geometry);

  ASSERT_GREATER_OR_EQUAL(geometry.size(), 3, ("Is it an area feature?"));

  return cw.GetMatchingAreaFeatureFromOSM(geometry);
}

uint64_t GetMwmCreationTimeByMwmId(MwmSet::MwmId const & mwmId)
{
  return mwmId.GetInfo()->m_version.GetSecondsSinceEpoch();
}

bool IsObsolete(editor::XMLFeature const & xml, FeatureID const & fid)
{
  // TODO(mgsergio): If xml and feature are identical return true
  auto const uploadTime = xml.GetUploadTime();
  return uploadTime != my::INVALID_TIME_STAMP &&
         my::TimeTToSecondsSinceEpoch(uploadTime) < GetMwmCreationTimeByMwmId(fid.m_mwmId);
}
} // namespace

namespace osm
{
// TODO(AlexZ): Normalize osm multivalue strings for correct merging
// (e.g. insert/remove spaces after ';' delimeter);

Editor::Editor() : m_configLoader(m_config), m_notes(editor::Notes::MakeNotes())
{
  SetDefaultStorage();
}

Editor & Editor::Instance()
{
  static Editor instance;
  return instance;
}

void Editor::SetDefaultStorage()
{
  m_storage = make_unique<editor::LocalStorage>();
}

void Editor::LoadMapEdits()
{
  if (!m_delegate)
  {
    LOG(LERROR, ("Can't load any map edits, delegate has not been set."));
    return;
  }

  xml_document doc;
  if (!m_storage->Load(doc))
    return;

  array<pair<FeatureStatus, char const *>, 4> const sections =
  {{
      {FeatureStatus::Deleted, kDeleteSection},
      {FeatureStatus::Modified, kModifySection},
      {FeatureStatus::Obsolete, kObsoleteSection},
      {FeatureStatus::Created, kCreateSection}
  }};
  int deleted = 0, obsolete = 0, modified = 0, created = 0;

  bool needRewriteEdits = false;

  // TODO(mgsergio): synchronize access to m_features.
  m_features.clear();
  for (xml_node mwm : doc.child(kXmlRootNode).children(kXmlMwmNode))
  {
    string const mapName = mwm.attribute("name").as_string("");
    int64_t const mapVersion = mwm.attribute("version").as_llong(0);
    MwmSet::MwmId const mwmId = GetMwmIdByMapName(mapName);
    // TODO(mgsergio, AlexZ): Is it normal to have isMwmIdAlive and mapVersion
    // NOT equal to mwmId.GetInfo()->GetVersion() at the same time?
    auto const needMigrateEdits = !mwmId.IsAlive() || mapVersion != mwmId.GetInfo()->GetVersion();
    needRewriteEdits |= needMigrateEdits;

    for (auto const & section : sections)
    {
      for (auto const nodeOrWay : mwm.child(section.second).select_nodes("node|way"))
      {
        try
        {
          XMLFeature const xml(nodeOrWay.node());

          // TODO(mgsergio): A map could be renamed, we'll treat it as deleted.
          // The right thing to do is to try to migrate all changes anyway.
          if (!mwmId.IsAlive())
          {
            LOG(LINFO, ("Mwm", mapName, "was deleted"));
            goto SECTION_END;
          }

          TForEachFeaturesNearByFn forEach = [this](TFeatureTypeFn && fn,
                                                    m2::PointD const & point) {
            return ForEachFeatureAtPoint(move(fn), point);
          };

          // TODO(mgsergio): Deleted features are not properly handled yet.
          auto const fid = needMigrateEdits
                               ? editor::MigrateFeatureIndex(
                                     forEach, xml, section.first,
                                     [this, &mwmId] { return GenerateNewFeatureId(mwmId); })
                               : FeatureID(mwmId, xml.GetMWMFeatureIndex());

          // Remove obsolete changes during migration.
          if (needMigrateEdits && IsObsolete(xml, fid))
            continue;

          FeatureTypeInfo fti;
          if (section.first == FeatureStatus::Created)
          {
            fti.m_feature.FromXML(xml);
          }
          else
          {
            auto const originalFeaturePtr = GetOriginalFeature(fid);
            if (!originalFeaturePtr)
            {
              LOG(LERROR, ("A feature with id", fid, "cannot be loaded."));
              alohalytics::LogEvent("Editor_MissingFeature_Error");
              goto SECTION_END;
            }

            fti.m_feature = *originalFeaturePtr;
            fti.m_feature.ApplyPatch(xml);
          }

          fti.m_feature.SetID(fid);
          fti.m_street = xml.GetTagValue(kAddrStreetTag);

          fti.m_modificationTimestamp = xml.GetModificationTime();
          ASSERT_NOT_EQUAL(my::INVALID_TIME_STAMP, fti.m_modificationTimestamp, ());
          fti.m_uploadAttemptTimestamp = xml.GetUploadTime();
          fti.m_uploadStatus = xml.GetUploadStatus();
          fti.m_uploadError = xml.GetUploadError();
          fti.m_status = section.first;
          switch (section.first)
          {
          case FeatureStatus::Deleted: ++deleted; break;
          case FeatureStatus::Modified: ++modified; break;
          case FeatureStatus::Obsolete: ++obsolete; break;
          case FeatureStatus::Created: ++created; break;
          case FeatureStatus::Untouched: ASSERT(false, ()); continue;
          }
          // Insert initialized structure at the end: exceptions are possible in above code.
          m_features[fid.m_mwmId].emplace(fid.m_index, move(fti));
        }
        catch (editor::XMLFeatureError const & ex)
        {
          ostringstream s;
          nodeOrWay.node().print(s, "  ");
          LOG(LERROR, (ex.what(), "Can't create XMLFeature in section", section.second, s.str()));
        }
        catch (editor::MigrationError const & ex)
        {
          LOG(LWARNING, (ex.Msg(), "mwmId =", mwmId, XMLFeature(nodeOrWay.node())));
        }
      } // for nodes
    } // for sections
 SECTION_END:
    ;
  } // for mwms

  // Save edits with new indexes and mwm version to avoid another migration on next startup.
  if (needRewriteEdits)
    Save();
  LOG(LINFO, ("Loaded", modified, "modified,",
              created, "created,", deleted, "deleted and", obsolete, "obsolete features."));
}

bool Editor::Save() const
{
  // TODO(AlexZ): Improve synchronization in Editor code.
  static mutex saveMutex;
  lock_guard<mutex> lock(saveMutex);

  if (m_features.empty())
  {
    m_storage->Reset();
    return true;
  }

  xml_document doc;
  xml_node root = doc.append_child(kXmlRootNode);
  // Use format_version for possible future format changes.
  root.append_attribute("format_version") = 1;
  for (auto const & mwm : m_features)
  {
    xml_node mwmNode = root.append_child(kXmlMwmNode);
    mwmNode.append_attribute("name") = mwm.first.GetInfo()->GetCountryName().c_str();
    mwmNode.append_attribute("version") = static_cast<long long>(mwm.first.GetInfo()->GetVersion());
    xml_node deleted = mwmNode.append_child(kDeleteSection);
    xml_node modified = mwmNode.append_child(kModifySection);
    xml_node created = mwmNode.append_child(kCreateSection);
    xml_node obsolete = mwmNode.append_child(kObsoleteSection);
    for (auto const & index : mwm.second)
    {
      FeatureTypeInfo const & fti = index.second;
      // TODO: Do we really need to serialize deleted features in full details? Looks like mwm ID and meta fields are enough.
      XMLFeature xf = fti.m_feature.ToXML(true /*type serializing helps during migration*/);
      xf.SetMWMFeatureIndex(index.first);
      if (!fti.m_street.empty())
        xf.SetTagValue(kAddrStreetTag, fti.m_street);
      ASSERT_NOT_EQUAL(0, fti.m_modificationTimestamp, ());
      xf.SetModificationTime(fti.m_modificationTimestamp);
      if (fti.m_uploadAttemptTimestamp != my::INVALID_TIME_STAMP)
      {
        xf.SetUploadTime(fti.m_uploadAttemptTimestamp);
        ASSERT(!fti.m_uploadStatus.empty(), ("Upload status updates with upload timestamp."));
        xf.SetUploadStatus(fti.m_uploadStatus);
        if (!fti.m_uploadError.empty())
          xf.SetUploadError(fti.m_uploadError);
      }
      switch (fti.m_status)
      {
      case FeatureStatus::Deleted: VERIFY(xf.AttachToParentNode(deleted), ()); break;
      case FeatureStatus::Modified: VERIFY(xf.AttachToParentNode(modified), ()); break;
      case FeatureStatus::Created: VERIFY(xf.AttachToParentNode(created), ()); break;
      case FeatureStatus::Obsolete: VERIFY(xf.AttachToParentNode(obsolete), ()); break;
      case FeatureStatus::Untouched: CHECK(false, ("Not edited features shouldn't be here."));
      }
    }
  }

  return m_storage->Save(doc);
}

void Editor::ClearAllLocalEdits()
{
  m_features.clear();
  Save();
  Invalidate();
}

void Editor::OnMapDeregistered(platform::LocalCountryFile const & localFile)
{
  // TODO: to add some synchronization mechanism for whole Editor class
  lock_guard<mutex> g(m_mapDeregisteredMutex);

  using TFeaturePair = decltype(m_features)::value_type;
  // Cannot search by MwmId because country already removed. So, search by country name.
  auto const matchedMwm =
      find_if(begin(m_features), end(m_features), [&localFile](TFeaturePair const & item) {
        return item.first.GetInfo()->GetCountryName() == localFile.GetCountryName();
      });

  if (m_features.end() != matchedMwm)
  {
    m_features.erase(matchedMwm);
    Save();
  }
}

Editor::FeatureStatus Editor::GetFeatureStatus(MwmSet::MwmId const & mwmId, uint32_t index) const
{
  // Most popular case optimization.
  if (m_features.empty())
    return FeatureStatus::Untouched;

  auto const * featureInfo = GetFeatureTypeInfo(mwmId, index);
  if (featureInfo == nullptr)
    return FeatureStatus::Untouched;

  return featureInfo->m_status;
}

Editor::FeatureStatus Editor::GetFeatureStatus(FeatureID const & fid) const
{
  return GetFeatureStatus(fid.m_mwmId, fid.m_index);
}

bool Editor::IsFeatureUploaded(MwmSet::MwmId const & mwmId, uint32_t index) const
{
  auto const * info = GetFeatureTypeInfo(mwmId, index);
  return info && info->m_uploadStatus == kUploaded;
}

void Editor::DeleteFeature(FeatureID const & fid)
{
  auto const mwm = m_features.find(fid.m_mwmId);
  if (mwm != m_features.end())
  {
    auto const f = mwm->second.find(fid.m_index);
    // Created feature is deleted by removing all traces of it.
    if (f != mwm->second.end() && f->second.m_status == FeatureStatus::Created)
    {
      mwm->second.erase(f);
      return;
    }
  }

  MarkFeatureWithStatus(fid, FeatureStatus::Deleted);

  // TODO(AlexZ): Synchronize Save call/make it on a separate thread.
  Save();
  Invalidate();
}

bool Editor::IsCreatedFeature(FeatureID const & fid)
{
  return feature::FakeFeatureIds::IsEditorCreatedFeature(fid.m_index);
}

bool Editor::OriginalFeatureHasDefaultName(FeatureID const & fid) const
{
  if (IsCreatedFeature(fid))
    return false;

  auto const originalFeaturePtr = GetOriginalFeature(fid);
  if (!originalFeaturePtr)
  {
    LOG(LERROR, ("A feature with id", fid, "cannot be loaded."));
    alohalytics::LogEvent("Editor_MissingFeature_Error");
    return false;
  }

  auto const & names = originalFeaturePtr->GetNames();

  return names.HasString(StringUtf8Multilang::kDefaultCode);
}

/// Several cases should be handled while saving changes:
/// 1) a feature is not in editor's cache
///   I. a feature was created
///      save it and mark as `Created'
///   II. a feature was modified
///      save it and mark as `Modified'
/// 2) a feature is in editor's cache
///   I. a feature was created
///      save it and mark as `Created'
///   II. a feature was modified and equals to the one in cache
///      ignore it
///   III. a feature was modified and equals to the one in mwm
///      either delete it or save and mark as `Modified' depending on upload status
Editor::SaveResult Editor::SaveEditedFeature(EditableMapObject const & emo)
{
  FeatureID const & fid = emo.GetID();
  FeatureTypeInfo fti;

  auto const featureStatus = GetFeatureStatus(fid.m_mwmId, fid.m_index);
  ASSERT_NOT_EQUAL(featureStatus, FeatureStatus::Obsolete, ("Obsolete feature cannot be modified."));
  ASSERT_NOT_EQUAL(featureStatus, FeatureStatus::Deleted, ("Unexpected feature status."));

  bool const wasCreatedByUser = IsCreatedFeature(fid);
  if (wasCreatedByUser)
  {
    fti.m_status = FeatureStatus::Created;
    fti.m_feature.ReplaceBy(emo);

    if (featureStatus == FeatureStatus::Created)
    {
      auto const & editedFeatureInfo = m_features[fid.m_mwmId][fid.m_index];
      if (AreFeaturesEqualButStreet(fti.m_feature, editedFeatureInfo.m_feature) &&
          emo.GetStreet().m_defaultName == editedFeatureInfo.m_street)
      {
        return NothingWasChanged;
      }
    }
  }
  else
  {
    auto const originalFeaturePtr = GetOriginalFeature(fid);
    if (!originalFeaturePtr)
    {
      LOG(LERROR, ("A feature with id", fid, "cannot be loaded."));
      alohalytics::LogEvent("Editor_MissingFeature_Error");
      return SaveResult::SavingError;
    }

    fti.m_feature = featureStatus == FeatureStatus::Untouched
        ? *originalFeaturePtr
        : m_features[fid.m_mwmId][fid.m_index].m_feature;
    fti.m_feature.ReplaceBy(emo);
    bool const sameAsInMWM =
        AreFeaturesEqualButStreet(fti.m_feature, *originalFeaturePtr) &&
        emo.GetStreet().m_defaultName == GetOriginalFeatureStreet(fti.m_feature);

    if (featureStatus != FeatureStatus::Untouched)
    {
      // A feature was modified and equals to the one in editor.
      auto const & editedFeatureInfo = m_features[fid.m_mwmId][fid.m_index];
      if (AreFeaturesEqualButStreet(fti.m_feature, editedFeatureInfo.m_feature) &&
          emo.GetStreet().m_defaultName == editedFeatureInfo.m_street)
      {
        return NothingWasChanged;
      }

      // A feature was modified and equals to the one in mwm (changes are rolled back).
      if (sameAsInMWM)
      {
        // Feature was not yet uploaded. Since it's equal to one mwm we can remove changes.
        if (editedFeatureInfo.m_uploadStatus != kUploaded)
        {
          if (!RemoveFeature(fid))
            return SavingError;

          return SavedSuccessfully;
        }
      }

      // If a feature is not the same as in mwm or it was uploaded
      // we must save it and mark for upload.
    }
    // A feature was NOT edited before and current changes are useless.
    else if (sameAsInMWM)
    {
      return NothingWasChanged;
    }

    fti.m_status = FeatureStatus::Modified;
  }

  // TODO: What if local client time is absolutely wrong?
  fti.m_modificationTimestamp = time(nullptr);
  fti.m_street = emo.GetStreet().m_defaultName;

  // Reset upload status so already uploaded features can be uploaded again after modification.
  fti.m_uploadStatus = {};
  m_features[fid.m_mwmId][fid.m_index] = move(fti);

  // TODO(AlexZ): Synchronize Save call/make it on a separate thread.
  bool const savedSuccessfully = Save();
  Invalidate();
  return savedSuccessfully ? SavedSuccessfully : NoFreeSpaceError;
}

bool Editor::RollBackChanges(FeatureID const & fid)
{
  if (IsFeatureUploaded(fid.m_mwmId, fid.m_index))
    return false;

  return RemoveFeature(fid);
}

void Editor::ForEachFeatureInMwmRectAndScale(MwmSet::MwmId const & id,
                                             TFeatureIDFunctor const & f,
                                             m2::RectD const & rect,
                                             int /*scale*/)
{
  auto const mwmFound = m_features.find(id);
  if (mwmFound == m_features.end())
    return;

  // TODO(AlexZ): Check that features are visible at this scale.
  // Process only new (created) features.
  for (auto const & index : mwmFound->second)
  {
    FeatureTypeInfo const & ftInfo = index.second;
    if (ftInfo.m_status == FeatureStatus::Created &&
        rect.IsPointInside(ftInfo.m_feature.GetCenter()))
      f(FeatureID(id, index.first));
  }
}

void Editor::ForEachFeatureInMwmRectAndScale(MwmSet::MwmId const & id,
                                             TFeatureTypeFunctor const & f,
                                             m2::RectD const & rect,
                                             int /*scale*/)
{
  auto mwmFound = m_features.find(id);
  if (mwmFound == m_features.end())
    return;

  // TODO(AlexZ): Check that features are visible at this scale.
  // Process only new (created) features.
  for (auto & index : mwmFound->second)
  {
    FeatureTypeInfo & ftInfo = index.second;
    if (ftInfo.m_status == FeatureStatus::Created &&
        rect.IsPointInside(ftInfo.m_feature.GetCenter()))
      f(ftInfo.m_feature);
  }
}

bool Editor::GetEditedFeature(MwmSet::MwmId const & mwmId, uint32_t index,
                              FeatureType & outFeature) const
{
  auto const * featureInfo = GetFeatureTypeInfo(mwmId, index);
  if (featureInfo == nullptr)
    return false;

  outFeature = featureInfo->m_feature;
  return true;
}

bool Editor::GetEditedFeature(FeatureID const & fid, FeatureType & outFeature) const
{
  return GetEditedFeature(fid.m_mwmId, fid.m_index, outFeature);
}

bool Editor::GetEditedFeatureStreet(FeatureID const & fid, string & outFeatureStreet) const
{
  auto const * featureInfo = GetFeatureTypeInfo(fid.m_mwmId, fid.m_index);
  if (featureInfo == nullptr)
    return false;

  outFeatureStreet = featureInfo->m_street;
  return true;
}

vector<uint32_t> Editor::GetFeaturesByStatus(MwmSet::MwmId const & mwmId, FeatureStatus status) const
{
  vector<uint32_t> features;
  auto const matchedMwm = m_features.find(mwmId);
  if (matchedMwm == m_features.end())
    return features;
  for (auto const & index : matchedMwm->second)
  {
    if (index.second.m_status == status)
      features.push_back(index.first);
  }
  sort(features.begin(), features.end());
  return features;
}

EditableProperties Editor::GetEditableProperties(FeatureType const & feature) const
{
  ASSERT(version::IsSingleMwm(feature.GetID().m_mwmId.GetInfo()->m_version.GetVersion()),
         ("Edit mode should be available only on new data"));

  ASSERT(GetFeatureStatus(feature.GetID()) != FeatureStatus::Obsolete,
         ("Edit mode should not be available on obsolete features"));

  // TODO(mgsergio): Check if feature is in the area where editing is disabled in the config.
  auto editableProperties = GetEditablePropertiesForTypes(feature::TypesHolder(feature));

  // Disable opening hours editing if opening hours cannot be parsed.
  if (GetFeatureStatus(feature.GetID()) != FeatureStatus::Created)
  {
    auto const originalFeaturePtr = GetOriginalFeature(feature.GetID());
    if (!originalFeaturePtr)
    {
      LOG(LERROR, ("A feature with id", feature.GetID(), "cannot be loaded."));
      alohalytics::LogEvent("Editor_MissingFeature_Error");
      return {};
    }

    auto const & metadata = originalFeaturePtr->GetMetadata();
    auto const & featureOpeningHours = metadata.Get(feature::Metadata::FMD_OPEN_HOURS);
    // Note: empty string is parsed as a valid opening hours rule.
    if (!osmoh::OpeningHours(featureOpeningHours).IsValid())
    {
      auto & meta = editableProperties.m_metadata;
      auto const toBeRemoved = remove(begin(meta), end(meta), feature::Metadata::FMD_OPEN_HOURS);
      if (toBeRemoved != end(meta))
        meta.erase(toBeRemoved);
    }
  }

  return editableProperties;
}
// private
EditableProperties Editor::GetEditablePropertiesForTypes(feature::TypesHolder const & types) const
{
  editor::TypeAggregatedDescription desc;
  if (m_config.Get()->GetTypeDescription(types.ToObjectNames(), desc))
    return {desc.GetEditableFields(), desc.IsNameEditable(), desc.IsAddressEditable()};
  return {};
}

bool Editor::HaveMapEditsToUpload() const
{
  for (auto const & id : m_features)
  {
    for (auto const & index : id.second)
    {
      if (NeedsUpload(index.second.m_uploadStatus))
        return true;
    }
  }
  return false;
}

bool Editor::HaveMapEditsOrNotesToUpload() const
{
  if (m_notes->NotUploadedNotesCount() != 0)
    return true;

  return HaveMapEditsToUpload();
}

bool Editor::HaveMapEditsToUpload(MwmSet::MwmId const & mwmId) const
{
  auto const found = m_features.find(mwmId);
  if (found != m_features.end())
  {
    for (auto const & index : found->second)
    {
      if (NeedsUpload(index.second.m_uploadStatus))
        return true;
    }
  }
  return false;
}

void Editor::UploadChanges(string const & key, string const & secret, TChangesetTags tags,
                           TFinishUploadCallback callBack)
{
  if (m_notes->NotUploadedNotesCount())
    UploadNotes(key, secret);

  if (!HaveMapEditsToUpload())
  {
    LOG(LDEBUG, ("There are no local edits to upload."));
    return;
  }

  alohalytics::LogEvent("Editor_DataSync_started");

  // TODO(AlexZ): features access should be synchronized.
  auto const upload = [this](string key, string secret, TChangesetTags tags, TFinishUploadCallback callBack)
  {
    // This lambda was designed to start after app goes into background. But for cases when user is immediately
    // coming back to the app we work with a copy, because 'for' loops below can take a significant amount of time.
    auto features = m_features;

    int uploadedFeaturesCount = 0, errorsCount = 0;
    ChangesetWrapper changeset({key, secret}, tags);
    for (auto & id : features)
    {
      for (auto & index : id.second)
      {
        FeatureTypeInfo & fti = index.second;
        // Do not process already uploaded features or those failed permanently.
        if (!NeedsUpload(fti.m_uploadStatus))
          continue;

        string ourDebugFeatureString;

        try
        {
          switch (fti.m_status)
          {
          case FeatureStatus::Untouched: CHECK(false, ("It's impossible.")); continue;
          case FeatureStatus::Obsolete: continue;  // Obsolete features will be deleted by OSMers.
          case FeatureStatus::Created:
            {
              XMLFeature feature = fti.m_feature.ToXML(true);
              if (!fti.m_street.empty())
                feature.SetTagValue(kAddrStreetTag, fti.m_street);
              ourDebugFeatureString = DebugPrint(feature);

              ASSERT_EQUAL(feature.GetType(), XMLFeature::Type::Node,
                           ("Linear and area features creation is not supported yet."));
              try
              {
                XMLFeature osmFeature = changeset.GetMatchingNodeFeatureFromOSM(fti.m_feature.GetCenter());
                // If we are here, it means that object already exists at the given point.
                // To avoid nodes duplication, merge and apply changes to it instead of creating an new one.
                XMLFeature const osmFeatureCopy = osmFeature;
                osmFeature.ApplyPatch(feature);
                // Check to avoid uploading duplicates into OSM.
                if (osmFeature == osmFeatureCopy)
                {
                  LOG(LWARNING, ("Local changes are equal to OSM, feature has not been uploaded.", osmFeatureCopy));
                  // Don't delete this local change right now for user to see it in profile.
                  // It will be automatically deleted by migration code on the next maps update.
                }
                else
                {
                  LOG(LDEBUG, ("Create case: uploading patched feature", osmFeature));
                  changeset.Modify(osmFeature);
                }
              }
              catch (ChangesetWrapper::OsmObjectWasDeletedException const &)
              {
                // Object was never created by anyone else - it's safe to create it.
                changeset.Create(feature);
              }
              catch (ChangesetWrapper::EmptyFeatureException const &)
              {
                // There is another node nearby, but it should be safe to create a new one.
                changeset.Create(feature);
              }
              catch (...)
              {
                // Pass network or other errors to outside exception handler.
                throw;
              }
            }
            break;

          case FeatureStatus::Modified:
            {
              // Do not serialize feature's type to avoid breaking OSM data.
              // TODO: Implement correct types matching when we support modifying existing feature types.
              XMLFeature feature = fti.m_feature.ToXML(false);
              if (!fti.m_street.empty())
                feature.SetTagValue(kAddrStreetTag, fti.m_street);
              ourDebugFeatureString = DebugPrint(feature);

              auto const originalFeaturePtr = GetOriginalFeature(fti.m_feature.GetID());
              if (!originalFeaturePtr)
              {
                LOG(LERROR, ("A feature with id", fti.m_feature.GetID(), "cannot be loaded."));
                alohalytics::LogEvent("Editor_MissingFeature_Error");
                RemoveFeatureFromStorageIfExists(fti.m_feature.GetID());
                continue;
              }

              XMLFeature osmFeature = GetMatchingFeatureFromOSM(
                  changeset, *originalFeaturePtr);
              XMLFeature const osmFeatureCopy = osmFeature;
              osmFeature.ApplyPatch(feature);
              // Check to avoid uploading duplicates into OSM.
              if (osmFeature == osmFeatureCopy)
              {
                LOG(LWARNING, ("Local changes are equal to OSM, feature has not been uploaded.", osmFeatureCopy));
                // Don't delete this local change right now for user to see it in profile.
                // It will be automatically deleted by migration code on the next maps update.
              }
              else
              {
                LOG(LDEBUG, ("Uploading patched feature", osmFeature));
                changeset.Modify(osmFeature);
              }
            }
            break;

          case FeatureStatus::Deleted:
            auto const originalFeaturePtr = GetOriginalFeature(fti.m_feature.GetID());
            if (!originalFeaturePtr)
            {
              LOG(LERROR, ("A feature with id", fti.m_feature.GetID(), "cannot be loaded."));
              alohalytics::LogEvent("Editor_MissingFeature_Error");
              RemoveFeatureFromStorageIfExists(fti.m_feature.GetID());
              continue;
            }
            changeset.Delete(GetMatchingFeatureFromOSM(
                changeset, *originalFeaturePtr));
            break;
          }
          fti.m_uploadStatus = kUploaded;
          fti.m_uploadError.clear();
          ++uploadedFeaturesCount;
        }
        catch (ChangesetWrapper::OsmObjectWasDeletedException const & ex)
        {
          fti.m_uploadStatus = kDeletedFromOSMServer;
          fti.m_uploadError = ex.Msg();
          ++errorsCount;
          LOG(LWARNING, (ex.what()));
        }
        catch (ChangesetWrapper::RelationFeatureAreNotSupportedException const & ex)
        {
          fti.m_uploadStatus = kRelationsAreNotSupported;
          fti.m_uploadError = ex.Msg();
          ++errorsCount;
          LOG(LWARNING, (ex.what()));
        }
        catch (ChangesetWrapper::EmptyFeatureException const & ex)
        {
          fti.m_uploadStatus = kWrongMatch;
          fti.m_uploadError = ex.Msg();
          ++errorsCount;
          LOG(LWARNING, (ex.what()));
        }
        catch (RootException const & ex)
        {
          fti.m_uploadStatus = kNeedsRetry;
          fti.m_uploadError = ex.Msg();
          ++errorsCount;
          LOG(LWARNING, (ex.what()));
        }
        // TODO(AlexZ): Use timestamp from the server.
        fti.m_uploadAttemptTimestamp = time(nullptr);

        if (fti.m_uploadStatus != kUploaded)
        {
          ms::LatLon const ll = MercatorBounds::ToLatLon(feature::GetCenter(fti.m_feature));
          alohalytics::LogEvent("Editor_DataSync_error", {{"type", fti.m_uploadStatus},
                                {"details", fti.m_uploadError}, {"our", ourDebugFeatureString},
                                {"mwm", fti.m_feature.GetID().GetMwmName()},
                                {"mwm_version", strings::to_string(fti.m_feature.GetID().GetMwmVersion())}},
                                alohalytics::Location::FromLatLon(ll.lat, ll.lon));
        }
        // Call Save every time we modify each feature's information.
        SaveUploadedInformation(fti);
      }
    }

    alohalytics::LogEvent("Editor_DataSync_finished", {{"errors", strings::to_string(errorsCount)},
                          {"uploaded", strings::to_string(uploadedFeaturesCount)},
                          {"changeset", strings::to_string(changeset.GetChangesetId())}});
    if (callBack)
    {
      UploadResult result = UploadResult::NothingToUpload;
      if (uploadedFeaturesCount)
        result = UploadResult::Success;
      else if (errorsCount)
        result = UploadResult::Error;
      callBack(result);
    }
  };

  // Do not run more than one upload thread at a time.
  static auto future = async(launch::async, upload, key, secret, tags, callBack);
  auto const status = future.wait_for(milliseconds(0));
  if (status == future_status::ready)
    future = async(launch::async, upload, key, secret, tags, callBack);
}

void Editor::SaveUploadedInformation(FeatureTypeInfo const & fromUploader)
{
  // TODO(AlexZ): Correctly synchronize this call and Save() at the end.
  FeatureID const & fid = fromUploader.m_feature.GetID();
  auto id = m_features.find(fid.m_mwmId);
  if (id == m_features.end())
    return;  // Rare case: feature was deleted at the time of changes uploading.
  auto index = id->second.find(fid.m_index);
  if (index == id->second.end())
    return;  // Rare case: feature was deleted at the time of changes uploading.
  auto & fti = index->second;
  fti.m_uploadAttemptTimestamp = fromUploader.m_uploadAttemptTimestamp;
  fti.m_uploadStatus = fromUploader.m_uploadStatus;
  fti.m_uploadError = fromUploader.m_uploadError;
  Save();
}

// Macros is used to avoid code duplication.
#define GET_FEATURE_TYPE_INFO_BODY                                        \
  do                                                                      \
  {                                                                       \
    /* TODO(mgsergio): machedMwm should be synchronized. */               \
    auto const matchedMwm = m_features.find(mwmId);                       \
    if (matchedMwm == m_features.end())                                   \
      return nullptr;                                                     \
                                                                          \
    auto const matchedIndex = matchedMwm->second.find(index);             \
    if (matchedIndex == matchedMwm->second.end())                         \
      return nullptr;                                                     \
                                                                          \
    /* TODO(AlexZ): Should we process deleted/created features as well?*/ \
    return &matchedIndex->second;                                         \
  } while (false)

Editor::FeatureTypeInfo const * Editor::GetFeatureTypeInfo(MwmSet::MwmId const & mwmId,
                                                           uint32_t index) const
{
  GET_FEATURE_TYPE_INFO_BODY;
}

Editor::FeatureTypeInfo * Editor::GetFeatureTypeInfo(MwmSet::MwmId const & mwmId, uint32_t index)
{
  GET_FEATURE_TYPE_INFO_BODY;
}

#undef GET_FEATURE_TYPE_INFO_BODY

void Editor::RemoveFeatureFromStorageIfExists(MwmSet::MwmId const & mwmId, uint32_t index)
{
  auto matchedMwm = m_features.find(mwmId);
  if (matchedMwm == m_features.end())
    return;

  auto matchedIndex = matchedMwm->second.find(index);
  if (matchedIndex != matchedMwm->second.end())
    matchedMwm->second.erase(matchedIndex);

  if (matchedMwm->second.empty())
    m_features.erase(matchedMwm);
}

void Editor::RemoveFeatureFromStorageIfExists(FeatureID const & fid)
{
  return RemoveFeatureFromStorageIfExists(fid.m_mwmId, fid.m_index);
}

void Editor::Invalidate()
{
  if (m_invalidateFn)
    m_invalidateFn();
}

bool Editor::MarkFeatureAsObsolete(FeatureID const & fid)
{
  auto const featureStatus = GetFeatureStatus(fid);
  if (featureStatus != FeatureStatus::Untouched && featureStatus != FeatureStatus::Modified)
  {
    ASSERT(false, ("Only untouched and modified features can be made obsolete"));
    return false;
  }

  MarkFeatureWithStatus(fid, FeatureStatus::Obsolete);

  Invalidate();
  return Save();
}

bool Editor::RemoveFeature(FeatureID const & fid)
{
  RemoveFeatureFromStorageIfExists(fid.m_mwmId, fid.m_index);
  Invalidate();
  return Save();
}

Editor::Stats Editor::GetStats() const
{
  Stats stats;
  LOG(LDEBUG, ("Edited features status:"));
  for (auto const & id : m_features)
  {
    for (auto const & index : id.second)
    {
      Editor::FeatureTypeInfo const & fti = index.second;
      stats.m_edits.push_back(make_pair(FeatureID(id.first, index.first),
                                        fti.m_uploadStatus + " " + fti.m_uploadError));
      LOG(LDEBUG, (fti.m_uploadAttemptTimestamp == my::INVALID_TIME_STAMP
                   ? "NOT_UPLOADED_YET" : my::TimestampToString(fti.m_uploadAttemptTimestamp), fti.m_uploadStatus,
                   fti.m_uploadError, fti.m_feature.GetFeatureType(), feature::GetCenter(fti.m_feature)));
      if (fti.m_uploadStatus == kUploaded)
      {
        ++stats.m_uploadedCount;
        if (stats.m_lastUploadTimestamp < fti.m_uploadAttemptTimestamp)
          stats.m_lastUploadTimestamp = fti.m_uploadAttemptTimestamp;
      }
    }
  }
  return stats;
}

NewFeatureCategories Editor::GetNewFeatureCategories() const
{
  return NewFeatureCategories(*(m_config.Get()));
}

FeatureID Editor::GenerateNewFeatureId(MwmSet::MwmId const & id)
{
  DECLARE_AND_ASSERT_THREAD_CHECKER("GenerateNewFeatureId is single-threaded.");
  // TODO(vng): Double-check if new feature indexes should uninterruptedly continue after existing indexes in mwm file.
  uint32_t featureIndex = feature::FakeFeatureIds::kEditorCreatedFeaturesStart;
  auto const found = m_features.find(id);
  if (found != m_features.end())
  {
    // Scan all already created features and choose next available ID.
    for (auto const & feature : found->second)
    {
      if (feature.second.m_status == FeatureStatus::Created && featureIndex <= feature.first)
        featureIndex = feature.first + 1;
    }
  }
  CHECK(feature::FakeFeatureIds::IsEditorCreatedFeature(featureIndex), ());
  return FeatureID(id, featureIndex);
}

bool Editor::CreatePoint(uint32_t type, m2::PointD const & mercator, MwmSet::MwmId const & id, EditableMapObject & outFeature)
{
  ASSERT(id.IsAlive(), ("Please check that feature is created in valid MWM file before calling this method."));
  if (!id.GetInfo()->m_limitRect.IsPointInside(mercator))
  {
    LOG(LERROR, ("Attempt to create a feature outside of the MWM's bounding box."));
    return false;
  }
  outFeature.SetMercator(mercator);
  outFeature.SetID(GenerateNewFeatureId(id));
  outFeature.SetType(type);
  outFeature.SetEditableProperties(GetEditablePropertiesForTypes(outFeature.GetTypes()));
  // Only point type features can be created at the moment.
  outFeature.SetPointType();
  return true;
}

void Editor::CreateNote(ms::LatLon const & latLon, FeatureID const & fid,
                        feature::TypesHolder const & holder, string const & defaultName,
                        NoteProblemType const type, string const & note)
{
  auto const version = GetMwmCreationTimeByMwmId(fid.m_mwmId);
  auto const stringVersion = my::TimestampToString(my::SecondsSinceEpochToTimeT(version));
  ostringstream sstr;
  auto canCreate = true;

  if (!note.empty())
    sstr << "\"" << note << "\"" << endl;

  switch (type)
  {
    case NoteProblemType::PlaceDoesNotExist:
    {
      sstr << kPlaceDoesNotExistMessage << endl;
      auto const isCreated = GetFeatureStatus(fid) == FeatureStatus::Created;
      auto const createdAndUploaded = (isCreated && IsFeatureUploaded(fid.m_mwmId, fid.m_index));
      CHECK(!isCreated || createdAndUploaded, ());

      if (createdAndUploaded)
        canCreate = RemoveFeature(fid);
      else
        canCreate = MarkFeatureAsObsolete(fid);

      break;
    }
    case NoteProblemType::General: break;
  }

  if (defaultName.empty())
    sstr << "POI has no name" << endl;
  else
    sstr << "POI name: " << defaultName << endl;

  sstr << "POI types:";
  for (auto const & type : holder.ToObjectNames())
  {
    sstr << ' ' << type;
  }
  sstr << endl;

  sstr << "OSM data version: " << stringVersion << endl;

  if (canCreate)
    m_notes->CreateNote(latLon, sstr.str());
}

void Editor::UploadNotes(string const & key, string const & secret)
{
  alohalytics::LogEvent("Editor_UploadNotes", strings::to_string(m_notes->NotUploadedNotesCount()));
  m_notes->Upload(OsmOAuth::ServerAuth({key, secret}));
}

void Editor::MarkFeatureWithStatus(FeatureID const & fid, FeatureStatus status)
{
  auto & fti = m_features[fid.m_mwmId][fid.m_index];

  auto const originalFeaturePtr = GetOriginalFeature(fid);

  if (!originalFeaturePtr)
  {
    LOG(LERROR, ("A feature with id", fid, "cannot be loaded."));
    alohalytics::LogEvent("Editor_MissingFeature_Error");
    return;
  }

  fti.m_feature = *originalFeaturePtr;
  fti.m_status = status;
  fti.m_modificationTimestamp = time(nullptr);
}

MwmSet::MwmId Editor::GetMwmIdByMapName(string const & name)
{
  if (!m_delegate)
  {
    LOG(LERROR, ("Can't get mwm id by map name:", name, ", delegate is not set."));
    return {};
  }
  return m_delegate->GetMwmIdByMapName(name);
}

unique_ptr<FeatureType> Editor::GetOriginalFeature(FeatureID const & fid) const
{
  if (!m_delegate)
  {
    LOG(LERROR, ("Can't get original feature by id:", fid, ", delegate is not set."));
    return {};
  }
  return m_delegate->GetOriginalFeature(fid);
}

string Editor::GetOriginalFeatureStreet(FeatureType & ft) const
{
  if (!m_delegate)
  {
    LOG(LERROR, ("Can't get feature street, delegate is not set."));
    return {};
  }
  return m_delegate->GetOriginalFeatureStreet(ft);
}

void Editor::ForEachFeatureAtPoint(TFeatureTypeFn && fn, m2::PointD const & point) const
{
  if (!m_delegate)
  {
    LOG(LERROR, ("Can't load features in point's vicinity, delegate is not set."));
    return;
  }
  m_delegate->ForEachFeatureAtPoint(move(fn), point);
}

string DebugPrint(Editor::FeatureStatus fs)
{
  switch (fs)
  {
  case Editor::FeatureStatus::Untouched: return "Untouched";
  case Editor::FeatureStatus::Deleted: return "Deleted";
  case Editor::FeatureStatus::Obsolete: return "Obsolete";
  case Editor::FeatureStatus::Modified: return "Modified";
  case Editor::FeatureStatus::Created: return "Created";
  };
}

const char * const Editor::kPlaceDoesNotExistMessage = "The place has gone or never existed. This is an auto-generated note from MAPS.ME application: a user reports a POI that is visible on a map (which can be outdated), but cannot be found on the ground.";
}  // namespace osm
