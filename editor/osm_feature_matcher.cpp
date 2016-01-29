#include "editor/osm_feature_matcher.hpp"

#include "base/logging.hpp"

using editor::XMLFeature;

namespace
{
constexpr double kPointDiffEps = MercatorBounds::GetCellID2PointAbsEpsilon();

bool LatLonEqual(ms::LatLon const & a, ms::LatLon const & b)
{
  return a.EqualDxDy(b, kPointDiffEps);
}

/// @returns value form (-Inf, 1]. Negative values are used as penalty,
/// positive as score.
double ScoreLatLon(XMLFeature const & xmlFt, ms::LatLon const & latLon)
{
  auto const a = MercatorBounds::FromLatLon(xmlFt.GetCenter());
  auto const b = MercatorBounds::FromLatLon(latLon);
  return 1.0 - (a.Length(b) / kPointDiffEps);
}

template <typename TFunc>
void ForEachWaysNode(pugi::xml_document const & osmResponse, pugi::xml_node const & way,
                     TFunc && func)
{
  for (auto const xNodeRef : way.select_nodes("nd/@ref"))
  {
    string const nodeRef = xNodeRef.attribute().value();
    auto const node = osmResponse.select_node(("osm/node[@id='" + nodeRef + "']").data()).node();
    ASSERT(node, ("OSM response have ref", nodeRef, "but have no node with such id.", osmResponse));
    XMLFeature xmlFt(node);
    func(xmlFt);
  }
}

/// @returns value form [-0.5, 0.5]. Negative values are used as penalty,
/// positive as score.
double ScoreGeometry(pugi::xml_document const & osmResponse, pugi::xml_node const & way,
                     vector<m2::PointD> geometry)
{
  int matched = 0;
  int total = 0;
  // TODO(mgsergio): optimize using sorting and scaning eps-squares.
  // The idea is to build squares with side = eps on points from the first set.
  // Sort them by y and x in y groups.
  // Then pass points from the second set through constructed struct and register events
  // like square started, square ended, point encounted.
  ForEachWaysNode(osmResponse, way, [&matched, &total, &geometry](XMLFeature const & xmlFt)
                  {
                    ++total;
                    for (auto pointIt = begin(geometry); pointIt != end(geometry); ++pointIt)
                    {
                      if (LatLonEqual(xmlFt.GetCenter(), MercatorBounds::ToLatLon(*pointIt)))
                      {
                        ++matched;
                        geometry.erase(pointIt);
                        break;
                      }
                    }
                  });

  return static_cast<double>(matched) / total - 0.5;
}
}  // namespace

namespace osm
{
pugi::xml_node GetBestOsmNode(pugi::xml_document const & osmResponse, ms::LatLon const & latLon)
{
  double bestScore = -1;
  pugi::xml_node bestMatchNode;

  for (auto const & xNode : osmResponse.select_nodes("osm/node"))
  {
    try
    {
      XMLFeature xmlFt(xNode.node());

      double const nodeScore = ScoreLatLon(xmlFt, latLon);
      if (nodeScore < 0)
        continue;

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

  // TODO(mgsergio): Add a properly defined threshold when more fields will be compared.
  // if (bestScore < kMiniScoreThreshold)
  //   return pugi::xml_node;

  return bestMatchNode;
}

pugi::xml_node GetBestOsmWay(pugi::xml_document const & osmResponse,
                             vector<m2::PointD> const & geometry)
{
  double bestScore = -1;
  pugi::xml_node bestMatchWay;

  for (auto const & xWay : osmResponse.select_nodes("osm/way"))
  {
    XMLFeature xmlFt(xWay.node());

    double const nodeScore = ScoreGeometry(osmResponse, xWay.node(), geometry);
    if (nodeScore < 0)
      continue;

    if (bestScore < nodeScore)
    {
      bestScore = nodeScore;
      bestMatchWay = xWay.node();
    }
  }

  // TODO(mgsergio): Add a properly defined threshold when more fields will be compared.
  // if (bestScore < kMiniScoreThreshold)
  //   return pugi::xml_node;

  return bestMatchWay;
}

}  // namespace osm
