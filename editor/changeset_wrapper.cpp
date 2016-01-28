#include "base/logging.hpp"

#include "indexer/feature.hpp"

#include "editor/changeset_wrapper.hpp"
#include "editor/osm_feature_matcher.hpp"

#include "std/algorithm.hpp"
#include "std/sstream.hpp"
#include "std/map.hpp"

#include "geometry/mercator.hpp"

#include "private.h"

using editor::XMLFeature;

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

XMLFeature ChangesetWrapper::GetMatchingNodeFeatureFromOSM(m2::PointD const & center)
{
  // Match with OSM node.
  ms::LatLon const ll = MercatorBounds::ToLatLon(center);
  pugi::xml_document doc;
  // Throws!
  LoadXmlFromOSM(ll, doc);

  pugi::xml_node const bestNode = GetBestOsmNode(doc, ll);
  if (bestNode.empty())
    MYTHROW(OsmObjectWasDeletedException,
            ("OSM does not have any nodes at the coordinates", ll, ", server has returned:", doc));

  return XMLFeature(bestNode);
}

XMLFeature ChangesetWrapper::GetMatchingAreaFeatureFromOSM(set<m2::PointD> const & geometry)
{
  // TODO: Make two/four requests using points on inscribed rectagle.
  for (auto const & pt : geometry)
  {
    ms::LatLon const ll = MercatorBounds::ToLatLon(pt);
    pugi::xml_document doc;
    // Throws!
    LoadXmlFromOSM(ll, doc);

    pugi::xml_node const bestWay = GetBestOsmWay(doc, geometry);
    if (bestWay.empty())
      continue;

    XMLFeature const way(bestWay);
    if (!way.IsArea())
      continue;

    // AlexZ: TODO: Check that this way is really match our feature.
    // If we had some way to check it, why not to use it in selecting our feature?

    return way;
  }
  MYTHROW(OsmObjectWasDeletedException,
          ("OSM does not have any matching way for feature"));
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
