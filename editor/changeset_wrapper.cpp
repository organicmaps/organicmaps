#include "indexer/feature.hpp"

#include "editor/changeset_wrapper.hpp"

#include "std/algorithm.hpp"

#include "private.h"

using editor::XMLFeature;

namespace osm
{

ChangesetWrapper::ChangesetWrapper(TKeySecret const & keySecret,
                                   ServerApi06::TKeyValueTags const & comments)
  : m_changesetComments(comments),
    // TODO(AlexZ): Replace with production server.
    m_api(OsmOAuth::IZServerAuth().SetToken(keySecret))
{
}

ChangesetWrapper::~ChangesetWrapper()
{
  if (m_changesetId)
    m_api.CloseChangeSet(m_changesetId);
}

XMLFeature ChangesetWrapper::GetMatchingFeatureFromOSM(XMLFeature const & ourPatch, FeatureType const & feature)
{
  if (feature.GetFeatureType() == feature::EGeomType::GEOM_POINT)
  {
    // Match with OSM node.
    ms::LatLon const ll = ourPatch.GetCenter();
    auto const response = m_api.GetXmlNodeByLatLon(ll.lat, ll.lon);
    if (response.first == OsmOAuth::ResponseCode::NetworkError)
      MYTHROW(NetworkErrorException, ("NetworkError with GetXmlNodeByLatLon request."));
    if (response.first != OsmOAuth::ResponseCode::OK)
      MYTHROW(HttpErrorException, ("HTTP error", response.first, "with GetXmlNodeByLatLon", ll));

    pugi::xml_document doc;
    if (pugi::status_ok != doc.load(response.second.c_str()).status)
      MYTHROW(OsmXmlParseException, ("Can't parse OSM server response for GetXmlNodeByLatLon request", response.second));

    // TODO(AlexZ): Select best matching OSM node, not just the first one.
    pugi::xml_node const firstNode = doc.child("osm").child("node");
    if (firstNode.empty())
      MYTHROW(OsmObjectWasDeletedException, ("OSM does not have any nodes at the coordinates", ll, ", server has returned:", response.second));

    return XMLFeature(firstNode);
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
      auto const response = m_api.GetXmlNodeByLatLon(ll.lat, ll.lon);
      if (response.first == OsmOAuth::ResponseCode::NetworkError)
        MYTHROW(NetworkErrorException, ("NetworkError with GetXmlNodeByLatLon request."));
      if (response.first != OsmOAuth::ResponseCode::OK)
        MYTHROW(HttpErrorException, ("HTTP error", response.first, "with GetXmlNodeByLatLon", ll));

      pugi::xml_document doc;
      if (pugi::status_ok != doc.load(response.second.c_str()).status)
        MYTHROW(OsmXmlParseException, ("Can't parse OSM server response for GetXmlNodeByLatLon request", response.second));

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
