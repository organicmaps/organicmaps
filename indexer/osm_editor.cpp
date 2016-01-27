#include "indexer/classificator.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/osm_editor.hpp"

#include "platform/platform.hpp"

#include "editor/changeset_wrapper.hpp"
#include "editor/osm_auth.hpp"
#include "editor/server_api.hpp"
#include "editor/xml_feature.hpp"

#include "coding/internal/file_data.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "std/algorithm.hpp"
#include "std/chrono.hpp"
#include "std/future.hpp"
#include "std/mutex.hpp"
#include "std/target_os.hpp"
#include "std/tuple.hpp"
#include "std/unordered_map.hpp"
#include "std/unordered_set.hpp"

#include "3party/pugixml/src/pugixml.hpp"

using namespace pugi;
using feature::EGeomType;
using feature::Metadata;
using editor::XMLFeature;

namespace
{
constexpr char const * kEditorXMLFileName = "edits.xml";
constexpr char const * kXmlRootNode = "mapsme";
constexpr char const * kXmlMwmNode = "mwm";
constexpr char const * kDeleteSection = "delete";
constexpr char const * kModifySection = "modify";
constexpr char const * kCreateSection = "create";
/// We store edited streets in OSM-compatible way.
constexpr char const * kAddrStreetTag = "addr:street";

constexpr char const * kUploaded = "Uploaded";
constexpr char const * kDeletedFromOSMServer = "Deleted from OSM by someone";
constexpr char const * kNeedsRetry = "Needs Retry";

bool NeedsUpload(string const & uploadStatus)
{
  return uploadStatus != kUploaded && uploadStatus != kDeletedFromOSMServer;
}

string GetEditorFilePath() { return GetPlatform().WritablePathForFile(kEditorXMLFileName); }
// TODO(mgsergio): Replace hard-coded value with reading from file.
/// type:string -> description:pair<fields:vector<???>, editName:bool, editAddr:bool>

using EType = feature::Metadata::EType;
using TEditableFields = vector<EType>;

struct TypeDescription
{
  TypeDescription(TEditableFields const & fields, bool const name, bool const address) :
      m_fields(fields),
      m_name(name),
      m_address(address)
  {
  }

  TEditableFields const m_fields;
  bool const m_name;
  // Address == true implies Street, House Number, Phone, Fax, Opening Hours, Website, EMail, Postcode.
  bool const m_address;
};

static unordered_map<string, TypeDescription> const gEditableTypes = {
  {"aeroway-aerodrome", {{EType::FMD_ELE, EType::FMD_OPERATOR}, false, true}},
  {"aeroway-airport", {{EType::FMD_ELE, EType::FMD_OPERATOR}, false, true}},
  {"amenity-atm", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE}, true, false}},
  {"amenity-bank", {{EType::FMD_OPERATOR}, true, true}},
  {"amenity-bar", {{EType::FMD_CUISINE, EType::FMD_INTERNET}, true, true}},
  {"amenity-bicycle_rental", {{EType::FMD_OPERATOR}, true, false}},
  {"amenity-bureau_de_change", {{EType::FMD_OPERATOR}, true, true}},
  {"amenity-bus_station", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"amenity-cafe", {{EType::FMD_CUISINE, EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"amenity-car_rental", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"amenity-car_sharing", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE}, true, false}},
  {"amenity-casino", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"amenity-cinema", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"amenity-college", {{EType::FMD_OPERATOR}, true, true}},
  {"amenity-doctors", {{EType::FMD_INTERNET}, true, true}},
  {"amenity-drinking_water", {{}, true, false}},
  {"amenity-embassy", {{}, true, true}},
  {"amenity-fast_food", {{EType::FMD_OPERATOR, EType::FMD_CUISINE, EType::FMD_INTERNET}, true, true}},
  {"amenity-ferry_terminal", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"amenity-fire_station", {{}, true, true}},
  {"amenity-fountain", {{}, true, false}},
  {"amenity-fuel", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"amenity-grave_yard", {{}, true, false}},
  {"amenity-hospital", {{}, true, true}},
  {"amenity-hunting_stand", {{EType::FMD_HEIGHT}, true, false}},
  {"amenity-kindergarten", {{EType::FMD_OPERATOR}, true, true}},
  {"amenity-library", {{EType::FMD_INTERNET}, true, true}},
  {"amenity-marketplace", {{EType::FMD_OPERATOR}, true, true}},
  {"amenity-nightclub", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"amenity-parking", {{EType::FMD_OPERATOR}, true, true}},
  {"amenity-pharmacy", {{EType::FMD_OPERATOR}, true, true}},
  {"amenity-place_of_worship", {{}, true, true}},
  {"amenity-police", {{}, true, true}},
  {"amenity-post_box", {{EType::FMD_OPERATOR, EType::FMD_POSTCODE}, true, false}},
  {"amenity-post_office", {{EType::FMD_OPERATOR, EType::FMD_POSTCODE, EType::FMD_INTERNET}, true, true}},
  {"amenity-pub", {{EType::FMD_OPERATOR, EType::FMD_CUISINE, EType::FMD_INTERNET}, true, true}},
  {"amenity-recycling", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE, EType::FMD_PHONE_NUMBER}, true, false}},
  {"amenity-restaurant", {{EType::FMD_OPERATOR, EType::FMD_CUISINE, EType::FMD_INTERNET}, true, true}},
  {"amenity-school", {{EType::FMD_OPERATOR}, true, true}},
  {"amenity-taxi", {{EType::FMD_OPERATOR}, true, false}},
  {"amenity-telephone", {{EType::FMD_OPERATOR, EType::FMD_PHONE_NUMBER}, false, false}},
  {"amenity-theatre", {{}, true, true}},
  {"amenity-toilets", {{EType::FMD_OPERATOR, EType::FMD_OPEN_HOURS}, true, false}},
  {"amenity-townhall", {{}, true, true}},
  {"amenity-university", {{}, true, true}},
  {"amenity-waste_disposal", {{EType::FMD_OPERATOR}, false, false}},
  {"highway-bus_stop", {{EType::FMD_OPERATOR}, true, false}},
  {"historic-archaeological_site", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"historic-castle", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"historic-memorial", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"historic-monument", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"historic-ruins", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"internet-access", {{EType::FMD_INTERNET}, false, false}},
  {"internet-access|wlan", {{EType::FMD_INTERNET}, false, false}},
  {"landuse-cemetery", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"leisure-garden", {{EType::FMD_OPEN_HOURS, EType::FMD_INTERNET}, true, false}},
  {"leisure-sports_centre", {{EType::FMD_INTERNET}, true, true}},
  {"leisure-stadium", {{EType::FMD_WIKIPEDIA, EType::FMD_OPERATOR}, true, true}},
  {"leisure-swimming_pool", {{EType::FMD_OPERATOR}, true, true}},
  {"natural-peak", {{EType::FMD_WIKIPEDIA, EType::FMD_ELE}, true, false}},
  {"natural-spring", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"natural-waterfall", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"office", {{EType::FMD_INTERNET}, true, true}},
  {"office-company", {{}, true, true}},
  {"office-government", {{}, true, true}},
  {"office-lawyer", {{}, true, true}},
  {"office-telecommunication", {{EType::FMD_INTERNET, EType::FMD_OPERATOR}, true, true}},
  {"place-farm", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"place-hamlet", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"place-village", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"railway-halt", {{}, true, false}},
  {"railway-station", {{EType::FMD_OPERATOR}, true, false}},
  {"railway-subway_entrance", {{}, true, false}},
  {"railway-tram_stop", {{EType::FMD_OPERATOR}, true, false}},
  {"shop", {{EType::FMD_INTERNET}, true, true}},
  {"shop-alcohol", {{EType::FMD_INTERNET}, true, true}},
  {"shop-bakery", {{EType::FMD_INTERNET}, true, true}},
  {"shop-beauty", {{EType::FMD_INTERNET}, true, true}},
  {"shop-beverages", {{EType::FMD_INTERNET}, true, true}},
  {"shop-bicycle", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"shop-books", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"shop-butcher", {{EType::FMD_INTERNET}, true, true}},
  {"shop-car", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"shop-car_repair", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"shop-chemist", {{EType::FMD_INTERNET}, true, true}},
  {"shop-clothes", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"shop-computer", {{EType::FMD_INTERNET}, true, true}},
  {"shop-confectionery", {{EType::FMD_INTERNET}, true, true }},
  {"shop-convenience", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"shop-department_store", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, false, true}},
  {"shop-doityourself", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"shop-electronics", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"shop-florist", {{EType::FMD_INTERNET}, true, true}},
  {"shop-furniture", {{EType::FMD_INTERNET}, false, true}},
  {"shop-garden_centre", {{EType::FMD_INTERNET}, true, true}},
  {"shop-gift", {{EType::FMD_INTERNET}, true, true}},
  {"shop-greengrocer", {{EType::FMD_INTERNET}, true, true}},
  {"shop-hairdresser", {{EType::FMD_INTERNET}, true, true}},
  {"shop-hardware", {{EType::FMD_INTERNET}, true, true}},
  {"shop-jewelry", {{EType::FMD_INTERNET}, true, true}},
  {"shop-kiosk", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"shop-laundry", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"shop-mall", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"shop-mobile_phone", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"shop-optician", {{EType::FMD_INTERNET}, true, true}},
  {"shop-shoes", {{EType::FMD_INTERNET}, true, true}},
  {"shop-sports", {{EType::FMD_INTERNET}, true, true}},
  {"shop-supermarket", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"shop-toys", {{EType::FMD_INTERNET}, true, true}},
  {"tourism-alpine_hut", {{EType::FMD_ELE, EType::FMD_OPEN_HOURS, EType::FMD_OPERATOR, EType::FMD_WEBSITE, EType::FMD_INTERNET}, true, false}},
  {"tourism-artwork", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"tourism-attraction", {{EType::FMD_WIKIPEDIA, EType::FMD_WEBSITE}, true, false}},
  {"tourism-camp_site", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE, EType::FMD_OPEN_HOURS, EType::FMD_INTERNET}, true, false}},
  {"tourism-caravan_site", {{EType::FMD_WEBSITE, EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, false}},
  {"tourism-guest_house", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"tourism-hostel", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"tourism-hotel", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"tourism-information", {{}, true, false}},
  {"tourism-motel", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"tourism-museum", {{EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"tourism-viewpoint", {{}, true, false}},
  {"waterway-waterfall", {{EType::FMD_HEIGHT}, true, false}}};

TypeDescription const * GetTypeDescription(uint32_t type, uint8_t typeTruncateLevel = 2)
{
  // Truncate is needed to match, for example, amenity-restaurant-vegan as amenity-restaurant.
  ftype::TruncValue(type, typeTruncateLevel);
  auto const readableType = classif().GetReadableObjectName(type);
  auto const it = gEditableTypes.find(readableType);
  if (it != end(gEditableTypes))
    return &it->second;
  return nullptr;
}

uint32_t MigrateFeatureIndex(XMLFeature const & /*xml*/)
{
  // @TODO(mgsergio): Update feature's index when user has downloaded fresh MWM file and old indices point to other features.
  // Possible implementation: use function to load features in rect (center feature's point) and somehow compare/choose from them.
  // Probably we need to store more data about features in xml, e.g. types, may be other data, to match them correctly.
  MYTHROW(RootException, ("TODO(mgsergio, AlexZ): Implement correct feature migrate code. Delete data/edits.xml to continue."));
  return 0;
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
} // namespace

namespace osm
{
// TODO(AlexZ): Normalize osm multivalue strings for correct merging
// (e.g. insert/remove spaces after ';' delimeter);

Editor & Editor::Instance()
{
  static Editor instance;
  return instance;
}

void Editor::LoadMapEdits()
{
  if (!m_mwmIdByMapNameFn)
  {
    LOG(LERROR, ("Can't load any map edits, MwmIdByNameAndVersionFn has not been set."));
    return;
  }

  xml_document doc;
  {
    string const fullFilePath = GetEditorFilePath();
    xml_parse_result const res = doc.load_file(fullFilePath.c_str());
    // Note: status_file_not_found is ok if user has never made any edits.
    if (res != status_ok && res != status_file_not_found)
    {
      LOG(LERROR, ("Can't load map edits from disk:", fullFilePath));
      return;
    }
  }

  array<pair<FeatureStatus, char const *>, 3> const sections =
  {{
      {FeatureStatus::Deleted, kDeleteSection},
      {FeatureStatus::Modified, kModifySection},
      {FeatureStatus::Created, kCreateSection}
  }};
  int deleted = 0, modified = 0, created = 0;

  for (xml_node mwm : doc.child(kXmlRootNode).children(kXmlMwmNode))
  {
    string const mapName = mwm.attribute("name").as_string("");
    int64_t const mapVersion = mwm.attribute("version").as_llong(0);
    MwmSet::MwmId const id = m_mwmIdByMapNameFn(mapName);
    if (!id.IsAlive())
    {
      // TODO(AlexZ): MWM file was deleted, but changes remain. What should we do in this case?
      LOG(LWARNING, (mapName, "version", mapVersion, "references not existing MWM file."));
      continue;
    }

    for (auto const & section : sections)
    {
      for (auto const nodeOrWay : mwm.child(section.second).select_nodes("node|way"))
      {
        try
        {
          XMLFeature const xml(nodeOrWay.node());
          // TODO(mgsergio, AlexZ): MigrateFeatureIndex() case will always throw now, so 'old' features are ignored.
          uint32_t const featureIndex = mapVersion == id.GetInfo()->GetVersion() ? xml.GetMWMFeatureIndex()
                                                                                 : MigrateFeatureIndex(xml);
          FeatureID const fid(id, featureIndex);

          FeatureTypeInfo & fti = m_features[id][fid.m_index];

          if (section.first == FeatureStatus::Created)
          {
            // TODO(mgsergio): Create features which are not present in mwm.
          }
          else
          {
            fti.m_feature = *m_getOriginalFeatureFn(fid);
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
          case FeatureStatus::Created: ++created; break;
          case FeatureStatus::Untouched: ASSERT(false, ()); break;
          }
        }
        catch (editor::XMLFeatureError const & ex)
        {
          ostringstream s;
          nodeOrWay.node().print(s, "  ");
          LOG(LERROR, (ex.what(), "Can't create XMLFeature in section", section.second, s.str()));
        }
      } // for nodes
    } // for sections
  } // for mwms

  LOG(LINFO, ("Loaded", modified, "modified,", created, "created and", deleted, "deleted features."));
}

void Editor::Save(string const & fullFilePath) const
{
  // TODO(AlexZ): Improve synchronization in Editor code.
  static mutex saveMutex;
  lock_guard<mutex> lock(saveMutex);

  if (m_features.empty())
  {
    my::DeleteFileX(GetEditorFilePath());
    return;
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
    for (auto const & index : mwm.second)
    {
      FeatureTypeInfo const & fti = index.second;
      XMLFeature xf = fti.m_feature.ToXML();
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
      case FeatureStatus::Untouched: CHECK(false, ("Not edited features shouldn't be here."));
      }
    }
  }

  if (doc)
  {
    auto const & tmpFileName = fullFilePath + ".tmp";
    if (!doc.save_file(tmpFileName.data(), "  "))
      LOG(LERROR, ("Can't save map edits into", tmpFileName));
    else if (!my::RenameFileX(tmpFileName, fullFilePath))
      LOG(LERROR, ("Can't rename file", tmpFileName, "to", fullFilePath));
  }
}

void Editor::ClearAllLocalEdits()
{
  m_features.clear();
  Save(GetEditorFilePath());
  m_invalidateFn();
}

Editor::FeatureStatus Editor::GetFeatureStatus(MwmSet::MwmId const & mwmId, uint32_t index) const
{
  // Most popular case optimization.
  if (m_features.empty())
    return FeatureStatus::Untouched;

  auto const matchedMwm = m_features.find(mwmId);
  if (matchedMwm == m_features.end())
    return FeatureStatus::Untouched;

  auto const matchedIndex = matchedMwm->second.find(index);
  if (matchedIndex == matchedMwm->second.end())
    return FeatureStatus::Untouched;

  return matchedIndex->second.m_status;
}

void Editor::DeleteFeature(FeatureType const & feature)
{
  FeatureID const fid = feature.GetID();
  FeatureTypeInfo & ftInfo = m_features[fid.m_mwmId][fid.m_index];
  ftInfo.m_status = FeatureStatus::Deleted;
  ftInfo.m_feature = feature;
  // TODO: What if local client time is absolutely wrong?
  ftInfo.m_modificationTimestamp = time(nullptr);

  // TODO(AlexZ): Synchronize Save call/make it on a separate thread.
  Save(GetEditorFilePath());

  Invalidate();
}

//namespace
//{
//FeatureID GenerateNewFeatureId(FeatureID const & oldFeatureId)
//{
//  // TODO(AlexZ): Stable & unique features ID generation.
//  static uint32_t newIndex = 0x0effffff;
//  return FeatureID(oldFeatureId.m_mwmId, newIndex++);
//}
//}  // namespace

void Editor::EditFeature(FeatureType & editedFeature, string const & editedStreet, string const & editedHouseNumber)
{
  // Check house number for validity.
  if (editedHouseNumber.empty() || feature::IsHouseNumber(editedHouseNumber))
    editedFeature.SetHouseNumber(editedHouseNumber);
  // TODO(AlexZ): Store edited house number as house name if feature::IsHouseNumber() returned false.

  FeatureID const fid = editedFeature.GetID();
  if (AreFeaturesEqualButStreet(editedFeature, *m_getOriginalFeatureFn(fid)) &&
      m_getOriginalFeatureStreetFn(editedFeature) == editedStreet)
  {
    RemoveFeatureFromStorageIfExists(fid.m_mwmId, fid.m_index);
    // TODO(AlexZ): Synchronize Save call/make it on a separate thread.
    Save(GetEditorFilePath());
    Invalidate();
    return;
  }

  FeatureTypeInfo fti;
  fti.m_status = FeatureStatus::Modified;
  fti.m_feature = editedFeature;
  // TODO: What if local client time is absolutely wrong?
  fti.m_modificationTimestamp = time(nullptr);
  fti.m_street = editedStreet;
  m_features[fid.m_mwmId][fid.m_index] = move(fti);

  // TODO(AlexZ): Synchronize Save call/make it on a separate thread.
  Save(GetEditorFilePath());
  Invalidate();
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
                                             uint32_t /*scale*/)
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

bool Editor::GetEditedFeature(MwmSet::MwmId const & mwmId, uint32_t index, FeatureType & outFeature) const
{
  auto const matchedMwm = m_features.find(mwmId);
  if (matchedMwm == m_features.end())
    return false;

  auto const matchedIndex = matchedMwm->second.find(index);
  if (matchedIndex == matchedMwm->second.end())
    return false;

  // TODO(AlexZ): Should we process deleted/created features as well?
  outFeature = matchedIndex->second.m_feature;
  return true;
}

bool Editor::GetEditedFeatureStreet(FeatureType const & feature, string & outFeatureStreet) const
{
  FeatureID const & fid = feature.GetID();
  // TODO(AlexZ): Reuse common code or better make better getters/setters for edited features.
  auto const matchedMwm = m_features.find(fid.m_mwmId);
  if (matchedMwm == m_features.end())
    return false;

  auto const matchedIndex = matchedMwm->second.find(fid.m_index);
  if (matchedIndex == matchedMwm->second.end())
    return false;

  // TODO(AlexZ): Should we process deleted/created features as well?
  outFeatureStreet = matchedIndex->second.m_street;
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

vector<Metadata::EType> Editor::EditableMetadataForType(FeatureType const & feature) const
{
  // TODO(mgsergio): Load editable fields into memory from XML and query them here.
  feature::TypesHolder const types(feature);
  set<Metadata::EType> fields;
  auto const & isBuilding = ftypes::IsBuildingChecker::Instance();
  for (auto type : types)
  {
    auto const * desc = GetTypeDescription(type);
    if (desc)
    {
      for (auto field : desc->m_fields)
        fields.insert(field);
      // If address is editable, many metadata fields are editable too.
      if (desc->m_address)
      {
        fields.insert(EType::FMD_EMAIL);
        fields.insert(EType::FMD_OPEN_HOURS);
        fields.insert(EType::FMD_PHONE_NUMBER);
        fields.insert(EType::FMD_WEBSITE);
      }
    }
    else if (isBuilding.HasTypeValue(type))
    {
      // Post boxes and post offices have editable postcode field defined separately.
      fields.insert(EType::FMD_POSTCODE);
    }
  }
  return {begin(fields), end(fields)};
}

bool Editor::IsNameEditable(FeatureType const & feature) const
{
  feature::TypesHolder const types(feature);
  for (auto type : types)
  {
    auto const * typeDesc = GetTypeDescription(type);
    if (typeDesc && typeDesc->m_name)
      return true;
  }
  return false;
}

bool Editor::IsAddressEditable(FeatureType const & feature) const
{
  feature::TypesHolder const types(feature);
  auto & isBuilding = ftypes::IsBuildingChecker::Instance();
  for (auto type : types)
  {
    // Building addresses are always editable.
    if (isBuilding.HasTypeValue(type))
      return true;
    auto const * typeDesc = GetTypeDescription(type);
    if (typeDesc && typeDesc->m_address)
      return true;
  }
  return false;
}

bool Editor::HaveSomethingToUpload() const
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

void Editor::UploadChanges(string const & key, string const & secret, TChangesetTags tags,
                           TFinishUploadCallback callBack)
{
  if (!HaveSomethingToUpload())
  {
    LOG(LDEBUG, ("There are no local edits to upload."));
    return;
  }
  {
    auto const stats = GetStats();
    tags["total_edits"] = strings::to_string(stats.m_edits.size());
    tags["uploaded_edits"] = strings::to_string(stats.m_uploadedCount);
    tags["created_by"] = "MAPS.ME " OMIM_OS_NAME;
  }
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

        // TODO(AlexZ): Create/delete nodes support.
        if (fti.m_status != FeatureStatus::Modified)
          continue;

        XMLFeature feature = fti.m_feature.ToXML();
        // TODO(AlexZ): Add areas(ways) upload support.
        if (feature.GetType() != XMLFeature::Type::Node)
          continue;

        try
        {
          XMLFeature osmFeature = changeset.GetMatchingFeatureFromOSM(feature, fti.m_feature);
          XMLFeature const osmFeatureCopy = osmFeature;
          osmFeature.ApplyPatch(feature);
          // Check to avoid duplicates.
          if (osmFeature == osmFeatureCopy)
          {
            LOG(LWARNING, ("Local changes are equal to OSM, feature was not uploaded, local changes were deleted.", feature));
            // TODO(AlexZ): Delete local change.
            continue;
          }
          LOG(LDEBUG, ("Uploading patched feature", osmFeature));
          changeset.ModifyNode(osmFeature);
          fti.m_uploadStatus = kUploaded;
          fti.m_uploadAttemptTimestamp = time(nullptr);
          fti.m_uploadError.clear();
          ++uploadedFeaturesCount;
        }
        catch (ChangesetWrapper::OsmObjectWasDeletedException const & ex)
        {
          fti.m_uploadStatus = kDeletedFromOSMServer;
          fti.m_uploadAttemptTimestamp = time(nullptr);
          fti.m_uploadError = ex.what();
          ++errorsCount;
          LOG(LWARNING, (ex.what()));
        }
        catch (RootException const & ex)
        {
          LOG(LWARNING, (ex.what()));
          fti.m_uploadStatus = kNeedsRetry;
          fti.m_uploadAttemptTimestamp = time(nullptr);
          fti.m_uploadError = ex.what();
          ++errorsCount;
          LOG(LWARNING, (ex.what()));
        }
        // Call Save every time we modify each feature's information.
        SaveUploadedInformation(fti);
      }
    }

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
  Save(GetEditorFilePath());
}

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

void Editor::Invalidate()
{
  if (m_invalidateFn)
    m_invalidateFn();
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

string DebugPrint(Editor::FeatureStatus fs)
{
  switch (fs)
  {
  case Editor::FeatureStatus::Untouched: return "Untouched";
  case Editor::FeatureStatus::Deleted: return "Deleted";
  case Editor::FeatureStatus::Modified: return "Modified";
  case Editor::FeatureStatus::Created: return "Created";
  };
}

}  // namespace osm
