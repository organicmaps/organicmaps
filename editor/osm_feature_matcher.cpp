#include "editor/osm_feature_matcher.hpp"

#include "base/logging.hpp"

#include "std/algorithm.hpp"

using editor::XMLFeature;

namespace
{
constexpr double kPointDiffEps = MercatorBounds::GetCellID2PointAbsEpsilon();

bool PointsEqual(m2::PointD const & a, m2::PointD const & b)
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

vector<m2::PointD> GetWaysGeometry(pugi::xml_document const & osmResponse,
                                   pugi::xml_node const & way)
{
  vector<m2::PointD> result;
  ForEachWaysNode(osmResponse, way, [&result](XMLFeature const & xmlFt)
                  {
                    result.push_back(xmlFt.GetMercatorCenter());
                  });
  return result;
}

/// @returns value form [-0.5, 0.5]. Negative values are used as penalty,
/// positive as score.
double ScoreGeometry(pugi::xml_document const & osmResponse, pugi::xml_node const & way,
                     vector<m2::PointD> geometry)
{
  int matched = 0;

  auto wayGeometry = GetWaysGeometry(osmResponse, way);

  sort(begin(wayGeometry), end(wayGeometry));
  sort(begin(geometry), end(geometry));

  auto it1 = begin(geometry);
  auto it2 = begin(wayGeometry);

  while (it1 != end(geometry) && it2 != end(wayGeometry))
  {
    if (PointsEqual(*it1, *it2))
    {
      ++matched;
      ++it1;
      ++it2;
    }
    else if (*it1 < *it2)
    {
      ++it1;
    }
    else
    {
      ++it2;
    }
  }

  auto const wayScore = static_cast<double>(matched) / wayGeometry.size() - 0.5;
  auto const geomScore = static_cast<double>(matched) / geometry.size() - 0.5;
  auto const result = wayScore <= 0 || geomScore <= 0
      ? -1
      : 2 / (1 / wayScore + 1 / geomScore);

  LOG(LDEBUG, ("Osm score:", wayScore, "our feature score:", geomScore,
               "Total score", result));

  return result;
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
    catch (editor::NoLatLon const & ex)
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

  // TODO(mgsergio): Handle relations as well. Put try_later=version status to edits.xml.
  for (auto const & xWay : osmResponse.select_nodes("osm/way"))
  {
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
