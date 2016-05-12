#include "editor/osm_feature_matcher.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"

using editor::XMLFeature;

namespace osm
{
using editor::XMLFeature;

constexpr double kPointDiffEps = 1e-5;

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

template <typename TFunc>
void ForEachRelationsNode(pugi::xml_document const & osmResponse, pugi::xml_node const & relation,
                          TFunc && func)
{
  for (auto const xNodeRef : relation.select_nodes("member[@type='way']/@ref"))
  {
    string const wayRef = xNodeRef.attribute().value();
    auto const xpath = "osm/way[@id='" + wayRef + "']";
    auto const way = osmResponse.select_node(xpath.data()).node();
    // Some ways can be missed from relation.
    if (!way)
      continue;
    ForEachWaysNode(osmResponse, way, forward<TFunc>(func));
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

vector<m2::PointD> GetRelationsGeometry(pugi::xml_document const & osmResponse,
                                        pugi::xml_node const & relation)
{
  vector<m2::PointD> result;
  ForEachRelationsNode(osmResponse, relation, [&result](XMLFeature const & xmlFt)
                       {
                         result.push_back(xmlFt.GetMercatorCenter());
                       });
  return result;
}

// TODO(mgsergio): XMLFeature should have GetGeometry method.
vector<m2::PointD> GetWaysOrRelationsGeometry(pugi::xml_document const & osmResponse,
                                              pugi::xml_node const & wayOrRelation)
{
  if (strcmp(wayOrRelation.name(), "way") == 0)
    return GetWaysGeometry(osmResponse, wayOrRelation);
  return GetRelationsGeometry(osmResponse, wayOrRelation);
}

/// @returns value form [-0.5, 0.5]. Negative values are used as penalty,
/// positive as score.
/// @param osmResponse - nodes, ways and relations from osm
/// @param wayOrRelation - either way or relation to be compared agains ourGeometry
/// @param outGeometry - geometry of a FeatureType (ourGeometry must be sort-uniqued)
double ScoreGeometry(pugi::xml_document const & osmResponse,
                     pugi::xml_node const & wayOrRelation, vector<m2::PointD> ourGeometry)
{
  ASSERT(!ourGeometry.empty(), ("Our geometry cannot be empty"));
  int matched = 0;

  auto theirGeometry = GetWaysOrRelationsGeometry(osmResponse, wayOrRelation);

  if (theirGeometry.empty())
    return -1;

  my::SortUnique(theirGeometry);

  auto ourIt = begin(ourGeometry);
  auto theirIt = begin(theirGeometry);

  while (ourIt != end(ourGeometry) && theirIt != end(theirGeometry))
  {
    if (PointsEqual(*ourIt, *theirIt))
    {
      ++matched;
      ++ourIt;
      ++theirIt;
    }
    else if (*ourIt < *theirIt)
    {
      ++ourIt;
    }
    else
    {
      ++theirIt;
    }
  }

  auto const wayScore = static_cast<double>(matched) / theirGeometry.size() - 0.5;
  auto const geomScore = static_cast<double>(matched) / ourGeometry.size() - 0.5;
  auto const result = wayScore <= 0 || geomScore <= 0
      ? -1
      : 2 / (1 / wayScore + 1 / geomScore);

  LOG(LDEBUG, ("Type:", wayOrRelation.name(), "Osm score:",
               wayScore, "our feature score:", geomScore, "Total score", result));

  return result;
}

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

pugi::xml_node GetBestOsmWayOrRelation(pugi::xml_document const & osmResponse,
                                       vector<m2::PointD> const & geometry)
{
  double bestScore = -1;
  pugi::xml_node bestMatchWay;

  auto const xpath = "osm/way|osm/relation[tag[@k='type' and @v='multipolygon']]";
  for (auto const & xWayOrRelation : osmResponse.select_nodes(xpath))
  {
    double const nodeScore = ScoreGeometry(osmResponse, xWayOrRelation.node(), geometry);
    if (nodeScore < 0)
      continue;

    if (bestScore < nodeScore)
    {
      bestScore = nodeScore;
      bestMatchWay = xWayOrRelation.node();
    }
  }

  // TODO(mgsergio): Add a properly defined threshold when more fields will be compared.
  // if (bestScore < kMiniScoreThreshold)
  //   return pugi::xml_node;

  return bestMatchWay;
}
}  // namespace osm
