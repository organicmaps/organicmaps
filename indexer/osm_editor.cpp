#include "indexer/classificator.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/osm_editor.hpp"

#include "platform/platform.hpp"

#include "editor/xml_feature.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "coding/internal/file_data.hpp"

#include "std/tuple.hpp"
#include "std/unordered_map.hpp"
#include "std/unordered_set.hpp"

#include "3party/pugixml/src/pugixml.hpp"

using namespace pugi;
using feature::EGeomType;
using feature::Metadata;
using editor::XMLFeature;

constexpr char const * kEditorXMLFileName = "edits.xml";
constexpr char const * kXmlRootNode = "mapsme";
constexpr char const * kXmlMwmNode = "mwm";
constexpr char const * kDeleteSection = "delete";
constexpr char const * kModifySection = "modify";
constexpr char const * kCreateSection = "create";
/// We store edited streets in OSM-compatible way.
constexpr char const * kAddrStreetTag = "addr:street";

namespace osm
{
// TODO(AlexZ): Normalize osm multivalue strings for correct merging
// (e.g. insert/remove spaces after ';' delimeter);

namespace
{
string GetEditorFilePath() { return GetPlatform().WritablePathForFile(kEditorXMLFileName); }
// TODO(mgsergio): Replace hard-coded value with reading from file.
/// type:string -> description:pair<fields:vector<???>, editName:bool, editAddr:bool>

using EType = feature::Metadata::EType;
using TEditableFields = vector<EType>;

struct TypeDescription
{
  TypeDescription(TEditableFields const & fields, bool const name, bool const address) :
      fields(fields),
      name(name),
      address(address)
  {
  }

  TEditableFields const fields;
  bool const name;
  bool const address;
};

static unordered_map<string, TypeDescription> const gEditableTypes = {
  {"aeroway-aerodrome", {{EType::FMD_ELE, EType::FMD_PHONE_NUMBER, EType::FMD_OPERATOR}, false, true}},
  {"aeroway-airport", {{EType::FMD_ELE, EType::FMD_PHONE_NUMBER, EType::FMD_OPERATOR}, false, true}},
  {"amenity-atm", {{}, true, false}},
  {"amenity-bank", {{EType::FMD_OPERATOR, EType::FMD_OPEN_HOURS, EType::FMD_WEBSITE, EType::FMD_OPERATOR}, true, true}},
  {"amenity-bar", {{EType::FMD_OPEN_HOURS}, true, true}},
  {"amenity-bicycle_rental", {{EType::FMD_OPERATOR}, false, true}},
  {"amenity-bureau_de_change", {{EType::FMD_OPEN_HOURS}, true, true}},
  {"amenity-bus_station", {{EType::FMD_OPERATOR}, true, false}},
  {"amenity-cafe", {{EType::FMD_OPEN_HOURS, EType::FMD_PHONE_NUMBER, EType::FMD_WEBSITE, EType::FMD_OPERATOR, EType::FMD_INTERNET}, true, true}},
  {"amenity-car_rental", {{EType::FMD_OPERATOR}, true, false}},
  {"amenity-car_sharing", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE}, true, false}},
  {"amenity-casino", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE, EType::FMD_OPEN_HOURS}, true, false}},
  {"amenity-cinema", {{EType::FMD_OPERATOR, EType::FMD_PHONE_NUMBER, EType::FMD_WEBSITE}, true, true}},
  {"amenity-college", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE, EType::FMD_WEBSITE}, true, true}},
  {"amenity-doctors", {{}, true, true}},
  {"amenity-drinking_water", {{EType::FMD_OPERATOR}, true, false}},
  {"amenity-embassy", {{EType::FMD_PHONE_NUMBER, EType::FMD_WEBSITE}, true, false}},
  {"amenity-fast_food", {{EType::FMD_OPERATOR, EType::FMD_CUISINE}, true, false}},
  {"amenity-ferry_terminal", {{EType::FMD_OPERATOR}, true, false}},
  {"amenity-fire_station", {{}, true, false}},
  {"amenity-fountain", {{}, true, false}},
  {"amenity-fuel", {{EType::FMD_OPERATOR, EType::FMD_OPEN_HOURS, EType::FMD_PHONE_NUMBER, EType::FMD_HEIGHT /*maxheight?*/, EType::FMD_WEBSITE}, true, true }},
  {"amenity-grave_yard", {{}, true, false}},
  {"amenity-hospital", {{EType::FMD_WEBSITE, EType::FMD_PHONE_NUMBER}, true, true}},
  {"amenity-hunting_stand", {{EType::FMD_HEIGHT}, false, false}},
  {"amenity-kindergarten", {{EType::FMD_WEBSITE, EType::FMD_PHONE_NUMBER, EType::FMD_OPERATOR,  EType::FMD_WEBSITE}, true, true}},
  {"amenity-library", {{EType::FMD_PHONE_NUMBER, EType::FMD_WEBSITE,  EType::FMD_FAX_NUMBER, EType::FMD_FAX_NUMBER, EType::FMD_EMAIL}, true, true}},
  {"amenity-marketplace", {{EType::FMD_OPERATOR, EType::FMD_OPEN_HOURS}, true, false}},
  {"amenity-nightclub", {{EType::FMD_WEBSITE, EType::FMD_PHONE_NUMBER, EType::FMD_OPEN_HOURS, EType::FMD_OPERATOR}, true, true}},
  {"amenity-parking", {{EType::FMD_OPERATOR}, true, false}},
  {"amenity-pharmacy", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE}, true, true}},
  {"amenity-place_of_worship", {{EType::FMD_OPEN_HOURS, EType::FMD_WEBSITE}, true, false}},
  {"amenity-police", {{EType::FMD_OPERATOR, EType::FMD_PHONE_NUMBER, EType::FMD_WEBSITE, EType::FMD_OPEN_HOURS}, true, true}},
  {"amenity-post_box", {{EType::FMD_OPERATOR}, true, false}},
  {"amenity-post_office", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE,  EType::FMD_PHONE_NUMBER}, true, true}},
  {"amenity-pub", {{EType::FMD_OPERATOR, EType::FMD_OPEN_HOURS, EType::FMD_CUISINE,  EType::FMD_PHONE_NUMBER, EType::FMD_EMAIL, EType::FMD_WEBSITE, EType::FMD_FAX_NUMBER}, true, true}},
  {"amenity-recycling", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE}, true, false}},
  {"amenity-restaurant", {{EType::FMD_OPERATOR, EType::FMD_CUISINE, EType::FMD_OPEN_HOURS, EType::FMD_PHONE_NUMBER, EType::FMD_WEBSITE}, true, true}},
  {"amenity-school", {{EType::FMD_OPERATOR,  EType::FMD_WIKIPEDIA}, true, true}},
  {"amenity-taxi", {{EType::FMD_OPERATOR, EType::FMD_PHONE_NUMBER}, true, false}},
  {"amenity-telephone", {{EType::FMD_OPERATOR, EType::FMD_PHONE_NUMBER}, false, false}},
  {"amenity-theatre", {{EType::FMD_OPERATOR,  EType::FMD_WEBSITE, EType::FMD_PHONE_NUMBER}, true, true}},
  {"amenity-toilets", {{EType::FMD_OPEN_HOURS, EType::FMD_OPERATOR}, true, false}},
  {"amenity-townhall", {{EType::FMD_OPERATOR}, true, true}},
  {"amenity-university", {{EType::FMD_OPERATOR,  EType::FMD_PHONE_NUMBER, EType::FMD_WEBSITE, EType::FMD_FAX_NUMBER, EType::FMD_EMAIL, }, true, true}},
  {"amenity-waste_disposal", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE}, false, false}},
  {"highway-bus_stop", {{EType::FMD_OPERATOR}, true, false}},
  {"historic-archaeological_site", {{}, true, false}},
  {"historic-castle", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"historic-memorial", {{}, true, false}},
  {"historic-monument", {{}, true, false}},
  {"historic-ruins", {{}, true, false}},
  {"internet-access", {{EType::FMD_INTERNET /*??*/}, false, false}},
  {"internet-access|wlan", {{EType::FMD_INTERNET /*??*/}, false, false}},
  {"landuse-cemetery", {{}, true, false}},
  {"leisure-garden", {{}, true, false}},
  {"leisure-sports_centre", {{}, true, true}},
  {"leisure-stadium", {{EType::FMD_WIKIPEDIA, EType::FMD_WEBSITE, EType::FMD_OPERATOR}, true, true}},
  {"leisure-swimming_pool", {{EType::FMD_OPEN_HOURS, EType::FMD_OPERATOR}, true, false}},
  {"natural-peak", {{EType::FMD_WIKIPEDIA, EType::FMD_ELE}, true, false}},
  {"natural-spring", {{}, true, false}},
  {"natural-waterfall", {{}, true, false}},
  {"office-company", {{}, true, false}},
  {"office-government", {{}, true, false}},
  {"office-lawyer", {{EType::FMD_OPEN_HOURS, EType::FMD_PHONE_NUMBER, EType::FMD_FAX_NUMBER, EType::FMD_WEBSITE, EType::FMD_EMAIL}, true, false}},
  {"office-telecommunication", {{EType::FMD_OPEN_HOURS, EType::FMD_OPERATOR}, true, false}},
  {"place-farm", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"place-hamlet", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"place-village", {{EType::FMD_WIKIPEDIA}, true, false}},
  {"railway-halt", {{}, true, false}},
  {"railway-station", {{EType::FMD_OPERATOR}, true, false}},
  {"railway-subway_entrance", {{}, true, false}},
  {"railway-tram_stop", {{EType::FMD_OPERATOR}, true, false}},
  {"shop-alcohol", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-bakery", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-beauty", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-beverages", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-bicycle", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE, EType::FMD_OPEN_HOURS}, true, true}},
  {"shop-books", {{EType::FMD_OPEN_HOURS, EType::FMD_OPERATOR}, true, false}},
  {"shop-butcher", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-car", {{EType::FMD_OPERATOR, EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-car_repair", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE, EType::FMD_PHONE_NUMBER}, true, true}},
  {"shop-chemist", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-clothes", {{EType::FMD_OPERATOR, EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-computer", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-confectionery", {{EType::FMD_OPEN_HOURS}, true, false }},
  {"shop-convenience", {{EType::FMD_OPERATOR, EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-department_store", {{EType::FMD_OPERATOR, EType::FMD_OPEN_HOURS}, false, false}},
  {"shop-doityourself", {{EType::FMD_OPERATOR, EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-electronics", {{EType::FMD_OPEN_HOURS, EType::FMD_OPERATOR}, true, false}},
  {"shop-florist", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-furniture", {{EType::FMD_OPEN_HOURS}, false, false}},
  {"shop-garden_centre", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-gift", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-greengrocer", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-hairdresser", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-hardware", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-jewelry", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-kiosk", {{EType::FMD_OPERATOR, EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-laundry", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-mall", {{EType::FMD_OPERATOR, EType::FMD_OPEN_HOURS}, true, true}},
  {"shop-mobile_phone", {{EType::FMD_OPERATOR, EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-optician", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-shoes", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-sports", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"shop-supermarket", {{EType::FMD_OPEN_HOURS, EType::FMD_OPERATOR}, true, false}},
  {"shop-toys", {{EType::FMD_OPEN_HOURS}, true, false}},
  {"tourism-alpine_hut", {{EType::FMD_ELE, EType::FMD_OPEN_HOURS, EType::FMD_OPERATOR}, true, false}},
  {"tourism-artwork", {{EType::FMD_WEBSITE, EType::FMD_WIKIPEDIA}, true, false}},
  {"tourism-camp_site", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE, EType::FMD_OPEN_HOURS}, true, false}},
  {"tourism-caravan_site", {{EType::FMD_WEBSITE, EType::FMD_OPERATOR}, true, false}},
  {"tourism-guest_house", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE}, true, false}},
  {"tourism-hostel", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE}, true, true}},
  {"tourism-hotel", {{EType::FMD_OPERATOR, EType::FMD_WEBSITE, EType::FMD_PHONE_NUMBER}, true, true}},
  {"tourism-information", {{}, true, false}},
  {"tourism-motel", {{EType::FMD_OPERATOR}, true, true}},
  {"tourism-museum", {{EType::FMD_OPERATOR, EType::FMD_OPEN_HOURS}, true, false}},
  {"tourism-viewpoint", {{}, true, false}},
  {"waterway-waterfall", {{EType::FMD_HEIGHT}, true, false}}};

TypeDescription const * GetTypeDescription(uint32_t const type)
{
  auto const readableType = classif().GetReadableObjectName(type);
  auto const it = gEditableTypes.find(readableType);
  if (it != end(gEditableTypes))
    return &it->second;
  return nullptr;
}

uint32_t MigrateFeatureOffset(XMLFeature const & /*xml*/)
{
  // @TODO(mgsergio): update feature's offset, user has downloaded fresh MWM file and old offsets point to other features.
  // Possible implementation: use function to load features in rect (center feature's point) and somehow compare/choose from them.
  // Probably we need to store more data about features in xml, e.g. types, may be other data, to match them correctly.
  return 0;
}

} // namespace

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
      // TODO(AlexZ): MWM file was deleted, but changes have left. What whould we do in this case?
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
          uint32_t const featureOffset = mapVersion < id.GetInfo()->GetVersion() ? xml.GetOffset() : MigrateFeatureOffset(xml);
          FeatureID const fid(id, featureOffset);

          FeatureTypeInfo fti;

          /// TODO(mgsergio): uncomment when feature creating will
          /// be required
          // if (xml.GetType() != XMLFeature::Type::Way)
          // {
          // TODO(mgsergio): Check if feature can be read.
          fti.m_feature = *m_featureLoaderFn(fid);
          fti.m_feature.ApplyPatch(xml);
          // }
          // else
          // {
          //   fti.m_feature = FeatureType::FromXML(xml);
          // }

          fti.m_feature.SetID(fid);
          fti.m_street = xml.GetTagValue(kAddrStreetTag);

          fti.m_modificationTimestamp = xml.GetModificationTime();
          ASSERT_NOT_EQUAL(my::INVALID_TIME_STAMP, fti.m_modificationTimestamp, ());
          fti.m_uploadAttemptTimestamp = xml.GetUploadTime();
          fti.m_uploadStatus = xml.GetUploadStatus();
          fti.m_uploadError = xml.GetUploadError();
          fti.m_status = section.first;

          /// Call to m_featureLoaderFn indirectly tries to load feature by
          /// it's ID from the editor's m_features.
          /// That's why insertion into m_features should go AFTER call to m_featureLoaderFn.
          m_features[id][fid.m_index] = fti;
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
  // Should we delete edits file if user has canceled all changes?
  if (m_features.empty())
    return;

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
    for (auto const & offset : mwm.second)
    {
      FeatureTypeInfo const & fti = offset.second;
      XMLFeature xf = fti.m_feature.ToXML();
      xf.SetOffset(offset.first);
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

Editor::FeatureStatus Editor::GetFeatureStatus(MwmSet::MwmId const & mwmId, uint32_t offset) const
{
  // Most popular case optimization.
  if (m_features.empty())
    return FeatureStatus::Untouched;

  auto const mwmMatched = m_features.find(mwmId);
  if (mwmMatched == m_features.end())
    return FeatureStatus::Untouched;

  auto const offsetMatched = mwmMatched->second.find(offset);
  if (offsetMatched == mwmMatched->second.end())
    return FeatureStatus::Untouched;

  return offsetMatched->second.m_status;
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

void Editor::EditFeature(FeatureType const & editedFeature, string const & editedStreet)
{
  // TODO(AlexZ): Check if feature has not changed and reset status.
  FeatureID const fid = editedFeature.GetID();
  FeatureTypeInfo & fti = m_features[fid.m_mwmId][fid.m_index];
  fti.m_status = FeatureStatus::Modified;
  fti.m_feature = editedFeature;
  // TODO: What if local client time is absolutely wrong?
  fti.m_modificationTimestamp = time(nullptr);

  if (!editedStreet.empty())
    fti.m_street = editedStreet;

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
    if (ftInfo.m_status == FeatureStatus::Created &&
        rect.IsPointInside(ftInfo.m_feature.GetCenter()))
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
    if (ftInfo.m_status == FeatureStatus::Created &&
        rect.IsPointInside(ftInfo.m_feature.GetCenter()))
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

vector<Metadata::EType> Editor::EditableMetadataForType(FeatureType const & feature) const
{
  // TODO(mgsergio): Load editable fields into memory from XML and query them here.
  feature::TypesHolder const types(feature);
  set<Metadata::EType> fields;
  for (auto type : types)
  {
    auto const * desc = GetTypeDescription(type);
    if (desc)
    {
      for (auto field : desc->fields)
        fields.insert(field);
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
    if (typeDesc && typeDesc->name)
      return true;
  }

  return false;
}

bool Editor::IsAddressEditable(FeatureType const & feature) const
{
  feature::TypesHolder const types(feature);
  for (auto type : types)
  {
    // Building addresses are always editable.
    if (ftypes::IsBuildingChecker::Instance().HasTypeValue(type))
      return true;
    auto const * typeDesc = GetTypeDescription(type);
    if (typeDesc && typeDesc->address)
      return true;
  }

  return false;
}
}  // namespace osm
