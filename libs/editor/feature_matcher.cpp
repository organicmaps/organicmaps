#include "editor/feature_matcher.hpp"

#include "geometry/intersection_score.hpp"
#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/stl_iterator.hpp"

#include <algorithm>
#include <functional>
#include <string>
#include <utility>

using editor::XMLFeature;

namespace
{
namespace bg = boost::geometry;

// Use simple xy coordinates because spherical are not supported by boost::geometry algorithms.
using PointXY = bg::model::d2::point_xy<double>;
using Polygon = bg::model::polygon<PointXY>;
using MultiPolygon = bg::model::multi_polygon<Polygon>;
using Linestring = bg::model::linestring<PointXY>;
using MultiLinestring = bg::model::multi_linestring<Linestring>;
using AreaType = bg::default_area_result<Polygon>::type;

using ForEachRefFn = std::function<void(XMLFeature const & xmlFt)>;
using ForEachWayFn = std::function<void(pugi::xml_node const & way, std::string const & role)>;

double const kPointDiffEps = 1e-5;

void AddInnerIfNeeded(pugi::xml_document const & osmResponse, pugi::xml_node const & way, Polygon & dest)
{
  if (dest.inners().empty() || dest.inners().back().empty())
    return;

  auto const refs = way.select_nodes("nd/@ref");
  if (refs.empty())
    return;

  std::string const nodeRef = refs[0].attribute().value();
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
  bool const needReverse = outerLines.size() > 1 && bg::equals(outerLines[0].front(), outerLines[1].back());

  for (size_t i = 0; i < outerLines.size(); ++i)
  {
    if (needReverse)
      bg::reverse(outerLines[i]);

    bg::append(dest.outer(), outerLines[i]);
  }
}

/// Returns value form (-Inf, 1]. Negative values are used as penalty, positive as score.
double ScoreLatLon(XMLFeature const & xmlFt, ms::LatLon const & latLon)
{
  auto const a = mercator::FromLatLon(xmlFt.GetCenter());
  auto const b = mercator::FromLatLon(latLon);
  return 1.0 - (a.Length(b) / kPointDiffEps);
}

void ForEachRefInWay(pugi::xml_document const & osmResponse, pugi::xml_node const & way, ForEachRefFn const & fn)
{
  for (auto const & xNodeRef : way.select_nodes("nd/@ref"))
  {
    std::string const nodeRef = xNodeRef.attribute().value();
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
    std::string const wayRef = xNodeRef.attribute().value();
    auto const xpath = "osm/way[@id='" + wayRef + "']";
    auto const way = osmResponse.select_node(xpath.c_str()).node();

    auto const rolePath = "member[@ref='" + wayRef + "']/@role";
    pugi::xpath_node roleNode = relation.select_node(rolePath.c_str());

    // It is possible to have a wayRef that refers to a way not included in a given relation.
    // We can skip such ways.
    if (!way)
      continue;

    // If more than one way is given and there is one with no role specified,
    // it's an error. We skip this particular way but try to use others anyway.
    if (!roleNode && nodesSet.size() != 1)
      continue;

    std::string const role = roleNode ? roleNode.attribute().value() : "outer";
    fn(way, role);
  }
}

template <typename Geometry>
void AppendWay(pugi::xml_document const & osmResponse, pugi::xml_node const & way, Geometry & dest)
{
  ForEachRefInWay(osmResponse, way, [&dest](XMLFeature const & xmlFt)
  {
    auto const & p = xmlFt.GetMercatorCenter();
    bg::append(dest, boost::make_tuple(p.x, p.y));
  });
}

Polygon GetWaysGeometry(pugi::xml_document const & osmResponse, pugi::xml_node const & way)
{
  Polygon result;
  AppendWay(osmResponse, way, result);

  bg::correct(result);

  return result;
}

Polygon GetRelationsGeometry(pugi::xml_document const & osmResponse, pugi::xml_node const & relation)
{
  Polygon result;
  MultiLinestring outerLines;

  auto const fn = [&osmResponse, &result, &outerLines](pugi::xml_node const & way, std::string const & role)
  {
    if (role == "outer")
    {
      outerLines.emplace_back();
      AppendWay(osmResponse, way, outerLines.back());
    }
    else if (role == "inner")
    {
      if (result.inners().empty())
        result.inners().emplace_back();

      // Support several inner rings.
      AddInnerIfNeeded(osmResponse, way, result);
      AppendWay(osmResponse, way, result.inners().back());
    }
  };

  ForEachWayInRelation(osmResponse, relation, fn);

  MakeOuterRing(outerLines, result);
  bg::correct(result);

  return result;
}

Polygon GetWaysOrRelationsGeometry(pugi::xml_document const & osmResponse, pugi::xml_node const & wayOrRelation)
{
  if (strcmp(wayOrRelation.name(), "way") == 0)
    return GetWaysGeometry(osmResponse, wayOrRelation);
  return GetRelationsGeometry(osmResponse, wayOrRelation);
}

/// Returns value form [-1, 1]. Negative values are used as penalty, positive as score.
/// |osmResponse| - nodes, ways and relations from osm;
/// |wayOrRelation| - either way or relation to be compared agains ourGeometry;
/// |ourGeometry| - geometry of a FeatureType;
double ScoreGeometry(pugi::xml_document const & osmResponse, pugi::xml_node const & wayOrRelation,
                     std::vector<m2::PointD> const & ourGeometry)
{
  ASSERT(!ourGeometry.empty(), ("Our geometry cannot be empty"));

  auto const their = GetWaysOrRelationsGeometry(osmResponse, wayOrRelation);

  if (bg::is_empty(their))
    return geometry::kPenaltyScore;

  auto const our = geometry::TrianglesToPolygon(ourGeometry);

  if (bg::is_empty(our))
    return geometry::kPenaltyScore;

  auto const score = geometry::GetIntersectionScore(our, their);

  // If area of the intersection is a half of the object area, penalty score will be returned.
  if (score <= 0.5)
    return geometry::kPenaltyScore;

  return score;
}
}  // namespace

namespace matcher
{
pugi::xml_node GetBestOsmNode(pugi::xml_document const & osmResponse, ms::LatLon const & latLon)
{
  double bestScore = geometry::kPenaltyScore;
  pugi::xml_node bestMatchNode;

  for (auto const & xNode : osmResponse.select_nodes("osm/node"))
  {
    try
    {
      XMLFeature const xmlFeature(xNode.node());

      double const nodeScore = ScoreLatLon(xmlFeature, latLon);
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

pugi::xml_node GetBestOsmWayOrRelation(pugi::xml_document const & osmResponse, std::vector<m2::PointD> const & geometry)
{
  double bestScore = geometry::kPenaltyScore;
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

double ScoreTriangulatedGeometries(std::vector<m2::PointD> const & lhs, std::vector<m2::PointD> const & rhs)
{
  auto const score = geometry::GetIntersectionScoreForTriangulated(lhs, rhs);

  // If area of the intersection is a half of the object area, penalty score will be returned.
  if (score <= 0.5)
    return geometry::kPenaltyScore;

  return score;
}

double ScoreTriangulatedGeometriesByPoints(std::vector<m2::PointD> const & lhs, std::vector<m2::PointD> const & rhs)
{
  // The default comparison operator used in sort above (cmp1) and one that is
  // used in set_itersection (cmp2) are compatible in that sence that
  // cmp2(a, b) :- cmp1(a, b) and
  // cmp1(a, b) :- cmp2(a, b) || a almost equal b.
  // You can think of cmp2 as !(a >= b).
  // But cmp2 is not transitive:
  // i.e. !cmp(a, b) && !cmp(b, c) does NOT implies !cmp(a, c),
  // |a, b| < eps, |b, c| < eps.
  // This could lead to unexpected results in set_itersection (with greedy implementation),
  // but we assume such situation is very unlikely.
  auto const matched = set_intersection(begin(lhs), end(lhs), begin(rhs), end(rhs), CounterIterator(),
                                        [](m2::PointD const & p1, m2::PointD const & p2)
  {
    return p1 < p2 && !p1.EqualDxDy(p2, mercator::kPointEqualityEps);
  }).GetCount();

  return static_cast<double>(matched) / static_cast<double>(lhs.size());
}
}  // namespace matcher
