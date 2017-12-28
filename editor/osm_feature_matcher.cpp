#include "editor/osm_feature_matcher.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"
#include "std/function.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

using editor::XMLFeature;

namespace
{
namespace bg = boost::geometry;

// Use simple xy coordinates because spherical are not supported by boost::geometry algorithms.
using SimplePoint = bg::model::d2::point_xy<double>;
using Polygon = bg::model::polygon<SimplePoint>;
using MultiPolygon = bg::model::multi_polygon<Polygon>;
using Linestring = bg::model::linestring<SimplePoint>;
using MultiLinestring = bg::model::multi_linestring<Linestring>;
using AreaType = bg::default_area_result<Polygon>::type;

using ForEachRefFn = function<void(XMLFeature const & xmlFt)>;
using ForEachWayFn = function<void(pugi::xml_node const & way, string const & role)>;

const double kPointDiffEps = 1e-5;
const double kPenaltyScore = -1;

AreaType Area(Polygon const & polygon)
{
  AreaType innerArea = 0.0;
  for (auto const & inn : polygon.inners())
  {
    innerArea += bg::area(inn);
  }

  return bg::area(polygon.outer()) - innerArea;
}

AreaType Area(MultiPolygon const & multipolygon)
{
  AreaType result = 0.0;
  for (auto const & polygon : multipolygon)
  {
    result += Area(polygon);
  }

  return result;
}

AreaType IntersectionArea(MultiPolygon const & our, Polygon const & their)
{
  AreaType intersectionArea = 0.0;
  MultiPolygon tmp;
  for (auto const & triangle : our)
  {
    bg::intersection(triangle, their.outer(), tmp);
    intersectionArea += bg::area(tmp);
    tmp.clear();
    for (auto const & inn : their.inners())
    {
      bg::intersection(triangle, inn, tmp);
      intersectionArea -= bg::area(tmp);
      tmp.clear();
    }
  }

  return intersectionArea;
}

void AddInnerIfNeeded(pugi::xml_document const & osmResponse, pugi::xml_node const & way,
                      Polygon & dest)
{
  if (dest.inners().empty() || dest.inners().back().empty())
    return;

  auto const refs = way.select_nodes("nd/@ref");
  if (refs.empty())
    return;

  string const nodeRef = refs[0].attribute().value();
  auto const node = osmResponse.select_node(("osm/node[@id='" + nodeRef + "']").data()).node();
  ASSERT(node, ("OSM response have ref", nodeRef, "but have no node with such id.", osmResponse));
  XMLFeature xmlFt(node);

  auto const & pt = dest.inners().back().back();
  m2::PointD lastPoint(pt.x(), pt.y());

  if (lastPoint.EqualDxDy(xmlFt.GetMercatorCenter(), kPointDiffEps))
    return;

  dest.inners().emplace_back();
}

void MakeOuterRing(MultiLinestring & outerLines, Polygon & dest)
{
  bool needReverse =
    outerLines.size() > 1 && bg::equals(outerLines[0].front(), outerLines[1].back());

  for (size_t i = 0; i < outerLines.size(); ++i)
  {
    if (needReverse)
      bg::reverse(outerLines[i]);

    bg::append(dest.outer(), outerLines[i]);
  }
}

void CorrectPolygon(Polygon & dest)
{
  bg::correct(dest.outer());

  for (auto & inn : dest.inners())
  {
    bg::correct(inn);
  }
}

double
MatchByGeometry(MultiPolygon const & our, Polygon const & their)
{
  if (!bg::is_valid(their.outer()))
    return kPenaltyScore;

  for (auto const & inn : their.inners())
  {
    if (!bg::is_valid(inn))
      return kPenaltyScore;
  }

  for (auto const & t : our)
  {
    if (!bg::is_valid(t))
      return kPenaltyScore;
  }

  auto const ourArea = Area(our);
  auto const theirArea = Area(their);
  auto const intersectionArea = IntersectionArea(our, their);
  auto const overlayArea = ourArea + theirArea - intersectionArea;

  // Avoid infinity.
  if(overlayArea == 0.0)
    return kPenaltyScore;

  auto const score = intersectionArea / overlayArea;

  // If area of the intersection is a half of the object area, penalty score will be returned.
  if (score <= 0.5)
    return kPenaltyScore;

  return score;
}

MultiPolygon TriangelsToPolygons(vector<m2::PointD> const & triangels)
{
  size_t const kTriangleSize = 3;
  CHECK_GREATER_OR_EQUAL(triangels.size(), kTriangleSize, ());

  MultiPolygon result;
  Polygon triangle;
  for (size_t i = 0; i < triangels.size(); ++i)
  {
    if (i % kTriangleSize == 0)
      result.emplace_back();

    bg::append(result.back(), boost::make_tuple(triangels[i].x, triangels[i].y));

    if ((i + 1) % kTriangleSize == 0)
      bg::correct(result.back());
  }

  return result;
}

/// @returns value form (-Inf, 1]. Negative values are used as penalty,
/// positive as score.
double ScoreLatLon(XMLFeature const & xmlFt, ms::LatLon const & latLon)
{
  auto const a = MercatorBounds::FromLatLon(xmlFt.GetCenter());
  auto const b = MercatorBounds::FromLatLon(latLon);
  return 1.0 - (a.Length(b) / kPointDiffEps);
}

void ForEachRefInWay(pugi::xml_document const & osmResponse, pugi::xml_node const & way,
                     ForEachRefFn const & fn)
{
  for (auto const xNodeRef : way.select_nodes("nd/@ref"))
  {
    string const nodeRef = xNodeRef.attribute().value();
    auto const node = osmResponse.select_node(("osm/node[@id='" + nodeRef + "']").data()).node();
    ASSERT(node, ("OSM response have ref", nodeRef, "but have no node with such id.", osmResponse));
    XMLFeature xmlFt(node);
    fn(xmlFt);
  }
}

void ForEachWayInRelation(pugi::xml_document const & osmResponse, pugi::xml_node const & relation,
                          ForEachWayFn const & fn)
{
  auto const nodesSet = relation.select_nodes("member[@type='way']/@ref");
  for (auto const & xNodeRef : nodesSet)
  {
    string const wayRef = xNodeRef.attribute().value();
    auto const xpath = "osm/way[@id='" + wayRef + "']";
    auto const way = osmResponse.select_node(xpath.c_str()).node();

    auto const rolePath = "member[@ref='" + wayRef + "']/@role";
    pugi::xpath_node role = relation.select_node(rolePath.c_str());

    // Some ways can be missed from relation and
    // we need to understand role of the way (inner/outer).
    if (!way || (!role && nodesSet.size() != 1))
      continue;

    fn(way, role.attribute().value());
  }
}

Polygon GetWaysGeometry(pugi::xml_document const & osmResponse, pugi::xml_node const & way)
{
  Polygon result;
  ForEachRefInWay(osmResponse, way, [&result](XMLFeature const & xmlFt)
  {
    auto const & p = xmlFt.GetMercatorCenter();
    bg::append(result, boost::make_tuple(p.x, p.y));
  });

  bg::correct(result);

  return result;
}

Polygon GetRelationsGeometry(pugi::xml_document const & osmResponse,
                             pugi::xml_node const & relation)
{
  Polygon result;
  MultiLinestring outerLines;

  auto const fn = [&osmResponse, &result, &outerLines](pugi::xml_node const & way,
                                                       string const & role)
  {
    if (role == "outer")
    {
      outerLines.emplace_back();
      ForEachRefInWay(osmResponse, way, [&outerLines](XMLFeature const & xmlFt)
      {
        auto const & p = xmlFt.GetMercatorCenter();
        bg::append(outerLines.back(), boost::make_tuple(p.x, p.y));
      });
    }
    else if (role == "inner")
    {
      if (result.inners().empty())
        result.inners().emplace_back();

      // Support several inner rings.
      AddInnerIfNeeded(osmResponse, way, result);

      ForEachRefInWay(osmResponse, way, [&result](XMLFeature const & xmlFt)
      {
        auto const & p = xmlFt.GetMercatorCenter();
        bg::append(result.inners().back(), boost::make_tuple(p.x, p.y));
      });
    }
  };

  ForEachWayInRelation(osmResponse, relation, fn);

  MakeOuterRing(outerLines, result);
  CorrectPolygon(result);

  return result;
}

Polygon GetWaysOrRelationsGeometry(pugi::xml_document const & osmResponse,
                                              pugi::xml_node const & wayOrRelation)
{
  if (strcmp(wayOrRelation.name(), "way") == 0)
    return GetWaysGeometry(osmResponse, wayOrRelation);
  return GetRelationsGeometry(osmResponse, wayOrRelation);
}

/// @returns value form [-1, 1]. Negative values are used as penalty, positive as score.
/// @param osmResponse - nodes, ways and relations from osm;
/// @param wayOrRelation - either way or relation to be compared agains ourGeometry;
/// @param outGeometry - geometry of a FeatureType (ourGeometry must be sort-uniqued);
double ScoreGeometry(pugi::xml_document const & osmResponse,
                     pugi::xml_node const & wayOrRelation, vector<m2::PointD> const & ourGeometry)
{
  ASSERT(!ourGeometry.empty(), ("Our geometry cannot be empty"));

  auto const their = GetWaysOrRelationsGeometry(osmResponse, wayOrRelation);

  if (bg::is_empty(their))
    return kPenaltyScore;

  auto const our = TriangelsToPolygons(ourGeometry);

  if (bg::is_empty(our))
    return kPenaltyScore;

  return MatchByGeometry(our, their);
}
} // namespace

namespace osm
{
pugi::xml_node GetBestOsmNode(pugi::xml_document const & osmResponse, ms::LatLon const & latLon)
{
  double bestScore = kPenaltyScore;
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
  double bestScore = kPenaltyScore;
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

  return bestMatchWay;
}
}  // namespace osm
