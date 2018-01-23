#include "editor/feature_matcher.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/stl_iterator.hpp"

#include "std/algorithm.hpp"
#include "std/function.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

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

using ForEachRefFn = function<void(XMLFeature const & xmlFt)>;
using ForEachWayFn = function<void(pugi::xml_node const & way, string const & role)>;

double const kPointDiffEps = 1e-5;
double const kPenaltyScore = -1;

template <typename LGeometry, typename RGeometry>
AreaType IntersectionArea(LGeometry const & our, RGeometry const & their)
{
  ASSERT(bg::is_valid(our), ());
  ASSERT(bg::is_valid(their), ());

  MultiPolygon result;
  bg::intersection(our, their, result);
  return bg::area(result);
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
  bool const needReverse =
      outerLines.size() > 1 && bg::equals(outerLines[0].front(), outerLines[1].back());

  for (size_t i = 0; i < outerLines.size(); ++i)
  {
    if (needReverse)
      bg::reverse(outerLines[i]);

    bg::append(dest.outer(), outerLines[i]);
  }
}

template <typename LGeometry, typename RGeometry>
double MatchByGeometry(LGeometry const & lhs, RGeometry const & rhs)
{
  if (!bg::is_valid(lhs) || !bg::is_valid(rhs))
    return kPenaltyScore;

  auto const lhsArea = bg::area(lhs);
  auto const rhsArea = bg::area(rhs);
  auto const intersectionArea = IntersectionArea(lhs, rhs);
  auto const unionArea = lhsArea + rhsArea - intersectionArea;

  // Avoid infinity.
  if (my::AlmostEqualAbs(unionArea, 0.0, 1e-18))
    return kPenaltyScore;

  auto const score = intersectionArea / unionArea;

  // If area of the intersection is a half of the object area, penalty score will be returned.
  if (score <= 0.5)
    return kPenaltyScore;

  return score;
}

MultiPolygon TrianglesToPolygon(vector<m2::PointD> const & points)
{
  size_t const kTriangleSize = 3;
  if (points.size() % kTriangleSize != 0 || points.empty())
    MYTHROW(matcher::NotAPolygonException, ("Count of points must be multiple of", kTriangleSize));

  vector<MultiPolygon> polygons;
  for (size_t i = 0; i < points.size(); i += kTriangleSize)
  {
    MultiPolygon polygon;
    polygon.resize(1);
    auto & p = polygon[0];
    auto & outer = p.outer();
    for (size_t j = i; j < i + kTriangleSize; ++j)
      outer.push_back(PointXY(points[j].x, points[j].y));
    bg::correct(p);
    if (!bg::is_valid(polygon))
      MYTHROW(matcher::NotAPolygonException, ("The triangle is not valid"));
    polygons.push_back(polygon);
  }

  if (polygons.empty())
    return {};

  auto & result = polygons[0];
  for (size_t i = 1; i < polygons.size(); ++i)
  {
    MultiPolygon u;
    bg::union_(result, polygons[i], u);
    u.swap(result);
  }
  return result;
}

/// Returns value form (-Inf, 1]. Negative values are used as penalty, positive as score.
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
    pugi::xpath_node roleNode = relation.select_node(rolePath.c_str());

    // It is possible to have a wayRef that refers to a way not included in a given relation.
    // We can skip such ways.
    if (!way)
      continue;

    // If more than one way is given and there is one with no role specified,
    // it's an error. We skip this particular way but try to use others anyway.
    if (!roleNode && nodesSet.size() != 1)
      continue;

    string role = "outer";
    if (roleNode)
      role = roleNode.attribute().value();

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

Polygon GetWaysOrRelationsGeometry(pugi::xml_document const & osmResponse,
                                   pugi::xml_node const & wayOrRelation)
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
                     vector<m2::PointD> const & ourGeometry)
{
  ASSERT(!ourGeometry.empty(), ("Our geometry cannot be empty"));

  auto const their = GetWaysOrRelationsGeometry(osmResponse, wayOrRelation);

  if (bg::is_empty(their))
    return kPenaltyScore;

  auto const our = TrianglesToPolygon(ourGeometry);

  if (bg::is_empty(our))
    return kPenaltyScore;

  return MatchByGeometry(our, their);
}
}  // namespace

namespace matcher
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

double ScoreTriangulatedGeometries(vector<m2::PointD> const & lhs, vector<m2::PointD> const & rhs)
{
  auto const lhsPolygon = TrianglesToPolygon(lhs);
  if (bg::is_empty(lhsPolygon))
    return kPenaltyScore;

  auto const rhsPolygon = TrianglesToPolygon(rhs);
  if (bg::is_empty(rhsPolygon))
    return kPenaltyScore;

  return MatchByGeometry(lhsPolygon, rhsPolygon);
}

double ScoreTriangulatedGeometriesByPoints(vector<m2::PointD> const & lhs,
                                           vector<m2::PointD> const & rhs)
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
  auto const matched = set_intersection(begin(lhs), end(lhs),
                                        begin(rhs), end(rhs),
                                        CounterIterator(),
                                        [](m2::PointD const & p1, m2::PointD const & p2)
                                        {
                                          return p1 < p2 && !p1.EqualDxDy(p2, 1e-7);
                                        }).GetCount();

  return static_cast<double>(matched) / lhs.size();
}
}  // namespace matcher
