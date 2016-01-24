#include "base/logging.hpp"

#include "indexer/feature.hpp"

#include "editor/changeset_wrapper.hpp"

#include "std/algorithm.hpp"
#include "std/sstream.hpp"
#include "std/map.hpp"

#include "geometry/mercator.hpp"

#include "private.h"

using editor::XMLFeature;

namespace
{
double ScoreLatLon(XMLFeature const & xmlFt, ms::LatLon const & latLon)
{
  double constexpr eps = MercatorBounds::GetCellID2PointAbsEpsilon();
  return latLon.EqualDxDy(xmlFt.GetCenter(), eps);
}

double ScoreNames(XMLFeature const & xmlFt, StringUtf8Multilang const & names)
{
  double score = 0;
  names.ForEach([&score, &xmlFt](uint8_t const langCode, string const & name)
                {
                  if (xmlFt.GetName(langCode) == name)
                    score += 1;
                  return true;
                });

  // TODO(mgsergio): Deafult name match should have greater wieght. Should it?

  return score;
}

vector<string> GetOsmOriginalTagsForType(feature::Metadata::EType const et)
{
  static multimap<feature::Metadata::EType, string> const kFmd2Osm = {
    {feature::Metadata::EType::FMD_CUISINE, "cuisine"},
    {feature::Metadata::EType::FMD_OPEN_HOURS, "opening_hours"},
    {feature::Metadata::EType::FMD_PHONE_NUMBER, "phone"},
    {feature::Metadata::EType::FMD_PHONE_NUMBER, "contact:phone"},
    {feature::Metadata::EType::FMD_FAX_NUMBER, "fax"},
    {feature::Metadata::EType::FMD_FAX_NUMBER, "contact:fax"},
    {feature::Metadata::EType::FMD_STARS, "stars"},
    {feature::Metadata::EType::FMD_OPERATOR, "operator"},
    {feature::Metadata::EType::FMD_URL, "url"},
    {feature::Metadata::EType::FMD_WEBSITE, "website"},
    {feature::Metadata::EType::FMD_WEBSITE, "contact:website"},
    // TODO: {feature::Metadata::EType::FMD_INTERNET, ""},
    {feature::Metadata::EType::FMD_ELE, "ele"},
    {feature::Metadata::EType::FMD_TURN_LANES, "turn:lanes"},
    {feature::Metadata::EType::FMD_TURN_LANES_FORWARD, "turn:lanes:forward"},
    {feature::Metadata::EType::FMD_TURN_LANES_BACKWARD, "turn:lanes:backward"},
    {feature::Metadata::EType::FMD_EMAIL, "email"},
    {feature::Metadata::EType::FMD_EMAIL, "contact:email"},
    {feature::Metadata::EType::FMD_POSTCODE, "addr:postcode"},
    {feature::Metadata::EType::FMD_WIKIPEDIA, "wikipedia"},
    {feature::Metadata::EType::FMD_MAXSPEED, "maxspeed"},
    {feature::Metadata::EType::FMD_FLATS, "addr:flats"},
    {feature::Metadata::EType::FMD_HEIGHT, "height"},
    {feature::Metadata::EType::FMD_MIN_HEIGHT, "min_height"},
    {feature::Metadata::EType::FMD_MIN_HEIGHT, "building:min_level"},
    {feature::Metadata::EType::FMD_DENOMINATION, "denomination"},
    {feature::Metadata::EType::FMD_BUILDING_LEVELS, "building:levels"},
  };

  vector<string> result;
  auto const range = kFmd2Osm.equal_range(et);
  for (auto it = range.first; it != range.second; ++it)
    result.emplace_back(it->second);

  return result;
}

double ScoreMetadata(XMLFeature const & xmlFt, feature::Metadata const & metadata)
{
  double score = 0;

  for (auto const type : metadata.GetPresentTypes())
  {
    for (auto const osm_tag : GetOsmOriginalTagsForType(static_cast<feature::Metadata::EType>(type)))
    {
      if (xmlFt.GetTagValue(osm_tag) == metadata.Get(type))
      {
        score += 1;
        break;
      }
    }
  }

  return score;
}

bool TypesEqual(XMLFeature const & xmlFt, feature::TypesHolder const & types)
{
  return false;
}

pugi::xml_node GetBestOsmNode(pugi::xml_document const & osmResponse, FeatureType const & ft)
{
  double bestScore = -1;
  pugi::xml_node bestMatchNode;

  for (auto const & xNode : osmResponse.select_nodes("node"))
  {
    try
    {
      XMLFeature xmlFt(xNode.node());

      if (!TypesEqual(xmlFt, feature::TypesHolder(ft)))
        continue;

      double nodeScore = ScoreLatLon(xmlFt, MercatorBounds::ToLatLon(ft.GetCenter()));
      if (nodeScore < 0)
        continue;

      nodeScore += ScoreNames(xmlFt, ft.GetNames());
      nodeScore += ScoreMetadata(xmlFt, ft.GetMetadata());

      if (bestScore < nodeScore)
      {
        bestScore = nodeScore;
        bestMatchNode = xNode.node();
      }
    }
    catch (editor::XMLFeatureNoLatLonError const & ex)
    {
      LOG(LWARNING, ("No lat/lon attribute in osm response node.", ex.Msg()));
      continue;
    }
  }

  // TODO(mgsergio): Add a properly defined threshold.
  // if (bestScore < minimumScoreThreshold)
  //   return pugi::xml_node;

  return bestMatchNode;
}
}  // namespace

string DebugPrint(pugi::xml_document const & doc)
{
  ostringstream stream;
  doc.print(stream, "  ");
  return stream.str();
}

namespace osm
{

ChangesetWrapper::ChangesetWrapper(TKeySecret const & keySecret,
                                   ServerApi06::TKeyValueTags const & comments)
  : m_changesetComments(comments),
    m_api(OsmOAuth::ServerAuth().SetToken(keySecret))
{
}

ChangesetWrapper::~ChangesetWrapper()
{
  if (m_changesetId)
    m_api.CloseChangeSet(m_changesetId);
}

void ChangesetWrapper::LoadXmlFromOSM(ms::LatLon const & ll, pugi::xml_document & doc)
{
  auto const response = m_api.GetXmlFeaturesAtLatLon(ll.lat, ll.lon);
  if (response.first == OsmOAuth::ResponseCode::NetworkError)
    MYTHROW(NetworkErrorException, ("NetworkError with GetXmlFeaturesAtLatLon request."));
  if (response.first != OsmOAuth::ResponseCode::OK)
    MYTHROW(HttpErrorException, ("HTTP error", response.first, "with GetXmlFeaturesAtLatLon", ll));

  if (pugi::status_ok != doc.load(response.second.c_str()).status)
    MYTHROW(OsmXmlParseException, ("Can't parse OSM server response for GetXmlFeaturesAtLatLon request", response.second));
}

XMLFeature ChangesetWrapper::GetMatchingFeatureFromOSM(XMLFeature const & ourPatch, FeatureType const & feature)
{
  if (feature.GetFeatureType() == feature::EGeomType::GEOM_POINT)
  {
    // Match with OSM node.
    ms::LatLon const ll = ourPatch.GetCenter();
    pugi::xml_document doc;
    // Throws!
    LoadXmlFromOSM(ll, doc);

    // feature must be the original one, not patched!
    pugi::xml_node const bestNode = GetBestOsmNode(doc, feature);
    if (bestNode.empty())
      MYTHROW(OsmObjectWasDeletedException, ("OSM does not have any nodes at the coordinates", ll, ", server has returned:", doc));

    return XMLFeature(bestNode);
  }
  else if (feature.GetFeatureType() == feature::EGeomType::GEOM_AREA)
  {
    using m2::PointD;
    // Set filters out duplicate points for closed ways or triangles' vertices.
    set<PointD> geometry;
    feature.ForEachTriangle([&geometry](PointD const & p1, PointD const & p2, PointD const & p3)
    {
      geometry.insert(p1);
      geometry.insert(p2);
      geometry.insert(p3);
    }, FeatureType::BEST_GEOMETRY);

    ASSERT_GREATER_OR_EQUAL(geometry.size(), 3, ("Is it an area feature?"));

    for (auto const & pt : geometry)
    {
      ms::LatLon const ll = MercatorBounds::ToLatLon(pt);
      pugi::xml_document doc;
      // Throws!
      LoadXmlFromOSM(ll, doc);

      // TODO(AlexZ): Select best matching OSM way from possible many ways.
      pugi::xml_node const firstWay = doc.child("osm").child("way");
      if (firstWay.empty())
        continue;

      XMLFeature const way(firstWay);
      if (!way.IsArea())
        continue;

      // TODO: Check that this way is really match our feature.

      return way;
    }
    MYTHROW(OsmObjectWasDeletedException, ("OSM does not have any matching way for feature", feature));
  }
  MYTHROW(LinearFeaturesAreNotSupportedException, ("We don't edit linear features yet."));
}

void ChangesetWrapper::ModifyNode(XMLFeature node)
{
  // TODO(AlexZ): ServerApi can be much better with exceptions.
  if (m_changesetId == kInvalidChangesetId && !m_api.CreateChangeSet(m_changesetComments, m_changesetId))
    MYTHROW(CreateChangeSetFailedException, ("CreateChangeSetFailedException"));

  uint64_t nodeId;
  if (!strings::to_uint64(node.GetAttribute("id"), nodeId))
    MYTHROW(CreateChangeSetFailedException, ("CreateChangeSetFailedException"));

  // Changeset id should be updated for every OSM server commit.
  node.SetAttribute("changeset", strings::to_string(m_changesetId));

  if (!m_api.ModifyNode(node.ToOSMString(), nodeId))
    MYTHROW(ModifyNodeFailedException, ("ModifyNodeFailedException"));
}

} // namespace osm
