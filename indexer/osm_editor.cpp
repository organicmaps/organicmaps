#include "indexer/classificator.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/index.hpp"
#include "indexer/osm_editor.hpp"

#include "platform/platform.hpp"

#include "editor/xml_feature.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/map.hpp"
#include "std/set.hpp"
#include "std/unordered_set.hpp"

#include <boost/functional/hash.hpp>

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

namespace osm
{

// TODO(AlexZ): Normalize osm multivalue strings for correct merging
// (e.g. insert/remove spaces after ';' delimeter);

namespace
{
string GetEditorFilePath() { return GetPlatform().WritablePathForFile(kEditorXMLFileName); }
// TODO(mgsergio): Replace hard-coded value with reading from file.
static unordered_set<string> const gEditableTypes = {
  {"aeroway-aerodrome"},
  {"aeroway-airport"},
  {"amenity-atm"},
  {"amenity-bank"},
  {"amenity-bar"},
  {"amenity-bbq"},
  {"amenity-bench"},
  {"amenity-bicycle_rental"},
  {"amenity-bureau_de_change"},
  {"amenity-bus_station"},
  {"amenity-cafe"},
  {"amenity-car_rental"},
  {"amenity-car_sharing"},
  {"amenity-casino"},
  {"amenity-cinema"},
  {"amenity-college"},
  {"amenity-doctors"},
  {"amenity-drinking_water"},
  {"amenity-embassy"},
  {"amenity-fast_food"},
  {"amenity-ferry_terminal"},
  {"amenity-fire_station"},
  {"amenity-fountain"},
  {"amenity-fuel"},
  {"amenity-grave_yard"},
  {"amenity-hospital"},
  {"amenity-hunting_stand"},
  {"amenity-kindergarten"},
  {"amenity-library"},
  {"amenity-marketplace"},
  {"amenity-nightclub"},
  {"amenity-parking"},
  {"amenity-pharmacy"},
  {"amenity-place_of_worship"},
  {"amenity-police"},
  {"amenity-post_box"},
  {"amenity-post_office"},
  {"amenity-pub"},
  {"amenity-recycling"},
  {"amenity-restaurant"},
  {"amenity-school"},
  {"amenity-shelter"},
  {"amenity-taxi"},
  {"amenity-telephone"},
  {"amenity-theatre"},
  {"amenity-toilets"},
  {"amenity-townhall"},
  {"amenity-university"},
  {"amenity-waste_disposal"},
  {"highway-bus_stop"},
  {"highway-speed_camera"},
  {"historic-archaeological_site"},
  {"historic-castle"},
  {"historic-memorial"},
  {"historic-monument"},
  {"historic-ruins"},
  {"internet-access"},
  {"internet-access|wlan"},
  {"landuse-cemetery"},
  {"leisure-garden"},
  {"leisure-pitch"},
  {"leisure-playground"},
  {"leisure-sports_centre"},
  {"leisure-stadium"},
  {"leisure-swimming_pool"},
  {"natural-peak"},
  {"natural-spring"},
  {"natural-waterfall"},
  {"office-company"},
  {"office-estate_agent"},
  {"office-government"},
  {"office-lawyer"},
  {"office-telecommunication"},
  {"place-farm"},
  {"place-hamlet"},
  {"place-village"},
  {"railway-halt"},
  {"railway-station"},
  {"railway-subway_entrance"},
  {"railway-tram_stop"},
  {"shop-alcohol"},
  {"shop-bakery"},
  {"shop-beauty"},
  {"shop-beverages"},
  {"shop-bicycle"},
  {"shop-books"},
  {"shop-butcher"},
  {"shop-car"},
  {"shop-car_repair"},
  {"shop-chemist"},
  {"shop-clothes"},
  {"shop-computer"},
  {"shop-confectionery"},
  {"shop-convenience"},
  {"shop-department_store"},
  {"shop-doityourself"},
  {"shop-electronics"},
  {"shop-florist"},
  {"shop-furniture"},
  {"shop-garden_centre"},
  {"shop-gift"},
  {"shop-greengrocer"},
  {"shop-hairdresser"},
  {"shop-hardware"},
  {"shop-jewelry"},
  {"shop-kiosk"},
  {"shop-laundry"},
  {"shop-mall"},
  {"shop-mobile_phone"},
  {"shop-optician"},
  {"shop-shoes"},
  {"shop-sports"},
  {"shop-supermarket"},
  {"shop-toys"},
  {"tourism-alpine_hut"},
  {"tourism-artwork"},
  {"tourism-attraction"},
  {"tourism-camp_site"},
  {"tourism-caravan_site"},
  {"tourism-guest_house"},
  {"tourism-hostel"},
  {"tourism-hotel"},
  {"tourism-information"},
  {"tourism-motel"},
  {"tourism-museum"},
  {"tourism-picnic_site"},
  {"tourism-viewpoint"},
  {"waterway-waterfall"}};

template <typename TIterator>
bool HasAtLeastOneEditableType(TIterator from, TIterator const to)
{
  while (from != to)
  {
    auto const & type = classif().GetReadableObjectName(*from++);
    if (gEditableTypes.find(type) != end(gEditableTypes))
      return true;
  }
  return false;
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
      // TODO(AlexZ): Handle case when map was upgraded and edits should migrate to fresh map data.
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
          FeatureID const fid(id, xml.GetOffset());
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

  if (doc && !doc.save_file(fullFilePath.c_str(), "  "))
    LOG(LERROR, ("Can't save map edits into", fullFilePath));
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

void Editor::EditFeature(FeatureType & editedFeature)
{
  // TODO(AlexZ): Check if feature has not changed and reset status.
  FeatureID const fid = editedFeature.GetID();
  FeatureTypeInfo & ftInfo = m_features[fid.m_mwmId][fid.m_index];
  ftInfo.m_status = FeatureStatus::Modified;
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
  TTypes types;
  feature.ForEachType([&types](uint32_t type) { types.push_back(type); });

  // Enable opening hours for the first release.
  if (HasAtLeastOneEditableType(begin(types), end(types)))
    return {Metadata::FMD_OPEN_HOURS};
  return {};
}
}  // namespace osm
