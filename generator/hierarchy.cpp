#include "generator/hierarchy.hpp"

#include "indexer/feature_algo.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iterator>
#include <limits>
#include <numeric>

#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
#include "std/boost_geometry.hpp"

BOOST_GEOMETRY_REGISTER_POINT_2D(m2::PointD, double, boost::geometry::cs::cartesian, x, y)
BOOST_GEOMETRY_REGISTER_RING(std::vector<m2::PointD>)

namespace generator
{
namespace hierarchy
{
using namespace feature;

namespace
{
double CalculateOverlapPercentage(std::vector<m2::PointD> const & lhs, std::vector<m2::PointD> const & rhs)
{
  if (!boost::geometry::intersects(lhs, rhs))
    return 0.0;

  using BoostPolygon = boost::geometry::model::polygon<m2::PointD>;
  std::vector<BoostPolygon> coll;
  boost::geometry::intersection(lhs, rhs, coll);
  auto const min = std::min(boost::geometry::area(lhs), boost::geometry::area(rhs));
  CHECK_GREATER(min, 0.0, (min));
  auto const binOp = [](double x, BoostPolygon const & y) { return x + boost::geometry::area(y); };
  auto const sum = std::accumulate(std::cbegin(coll), std::cend(coll), 0.0, binOp);
  return sum * 100 / min;
}
}  // namespace

bool FilterFeatureDefault(feature::FeatureBuilder const &)
{
  return true;
}

HierarchyPlace::HierarchyPlace(FeatureBuilder const & fb)
  : m_id(MakeCompositeId(fb))
  , m_name(fb.GetMultilangName())
  , m_types(fb.GetTypes())
  , m_rect(fb.GetLimitRect())
  , m_center(fb.GetKeyPoint())
{
  if (fb.IsPoint())
  {
    m_isPoint = true;
  }
  else if (fb.IsArea())
  {
    m_polygon = fb.GetOuterGeometry();
    boost::geometry::correct(m_polygon);
    m_area = boost::geometry::area(m_polygon);
  }
}

bool HierarchyPlace::Contains(HierarchyPlace const & smaller) const
{
  if (IsPoint())
    return false;

  if (smaller.IsPoint())
    return Contains(smaller.GetCenter());

  return smaller.GetArea() <= GetArea() && CalculateOverlapPercentage(m_polygon, smaller.m_polygon) > 80.0;
}

bool HierarchyPlace::Contains(m2::PointD const & point) const
{
  return boost::geometry::covered_by(point, m_polygon);
}

HierarchyLinker::HierarchyLinker(Node::Ptrs && nodes) : m_nodes(std::move(nodes)), m_tree(MakeTree4d(m_nodes)) {}

// static
HierarchyLinker::Tree4d HierarchyLinker::MakeTree4d(Node::Ptrs const & nodes)
{
  Tree4d tree;
  for (auto const & n : nodes)
    tree.Add(n, n->GetData().GetLimitRect());
  return tree;
}

HierarchyLinker::Node::Ptr HierarchyLinker::FindPlaceParent(HierarchyPlace const & place)
{
  Node::Ptr parent = nullptr;
  auto minArea = std::numeric_limits<double>::max();
  auto const point = place.GetCenter();
  m_tree.ForEachInRect({point, point}, [&](auto const & candidateNode)
  {
    // https://wiki.openstreetmap.org/wiki/Simple_3D_buildings
    // An object with tag 'building:part' is a part of a relation with outline 'building' or
    // is contained in a object with tag 'building'. This case is second. We suppose a building part is
    // only inside a building.
    static auto const & buildingChecker = ftypes::IsBuildingChecker::Instance();
    static auto const & buildingPartChecker = ftypes::IsBuildingPartChecker::Instance();
    auto const & candidate = candidateNode->GetData();
    if (buildingPartChecker(place.GetTypes()) &&
        !(buildingChecker(candidate.GetTypes()) || buildingPartChecker(candidate.GetTypes())))
    {
      return;
    }

    // A building part must have children only with 'building:part' type.
    if (!buildingPartChecker(place.GetTypes()) && buildingPartChecker(candidate.GetTypes()))
      return;

    if (place.GetCompositeId() == candidate.GetCompositeId())
      return;

    if (candidate.GetArea() < minArea && candidate.Contains(place))
    {
      // Sometimes there can be two places with the same geometry. We must check place node and
      // its parents to avoid cyclic connections.
      auto node = candidateNode;
      while (node->HasParent())
      {
        node = node->GetParent();
        if (node->GetData().GetCompositeId() == place.GetCompositeId())
          return;
      }

      parent = candidateNode;
      minArea = candidate.GetArea();
    }
  });
  return parent;
}

HierarchyLinker::Node::Ptrs HierarchyLinker::Link()
{
  for (auto & node : m_nodes)
  {
    auto const & place = node->GetData();
    auto const parentPlace = FindPlaceParent(place);
    if (!parentPlace)
      continue;

    tree_node::Link(node, parentPlace);
  }
  return m_nodes;
}

HierarchyEntryEnricher::HierarchyEntryEnricher(std::string const & osm2FtIdsPath, std::string const & countryFullPath)
  : m_featureGetter(countryFullPath)
{
  CHECK(m_osm2FtIds.ReadFromFile(osm2FtIdsPath), (osm2FtIdsPath));
}

std::optional<m2::PointD> HierarchyEntryEnricher::GetFeatureCenter(CompositeId const & id) const
{
  auto const optIds = m_osm2FtIds.GetFeatureIds(id);
  if (optIds.empty())
    return {};

  // A CompositeId id may correspond to several feature ids. These features can be represented by
  // three types of geometry. Logically, their centers coincide, but in practice they don’t,
  // because the centers are calculated differently. For example, for an object with a type area,
  // the area will be computed using the triangles geometry, but for an object with a type line,
  // the area will be computed using the outer geometry of a polygon.
  std::unordered_map<std::underlying_type_t<feature::GeomType>, m2::PointD> m;
  for (auto optId : optIds)
  {
    auto const ftPtr = m_featureGetter.GetFeatureByIndex(optId);
    if (!ftPtr)
      continue;

    CHECK(m.emplace(base::Underlying(ftPtr->GetGeomType()), feature::GetCenter(*ftPtr)).second, (id, optIds));
  }

  for (auto type : {base::Underlying(feature::GeomType::Point), base::Underlying(feature::GeomType::Area),
                    base::Underlying(feature::GeomType::Line)})
  {
    if (m.count(type) != 0)
      return m[type];
  }

  return {};
}

HierarchyLinesBuilder::HierarchyLinesBuilder(HierarchyLinker::Node::Ptrs && trees) : m_trees(std::move(trees)) {}

void HierarchyLinesBuilder::SetGetMainTypeFunction(GetMainTypeFn const & getMainType)
{
  m_getMainType = getMainType;
}

void HierarchyLinesBuilder::SetGetNameFunction(GetNameFn const & getName)
{
  m_getName = getName;
}

void HierarchyLinesBuilder::SetCountry(storage::CountryId const & country)
{
  m_countryName = country;
}

void HierarchyLinesBuilder::SetHierarchyEntryEnricher(std::unique_ptr<HierarchyEntryEnricher> && enricher)
{
  m_enricher = std::move(enricher);
}

std::vector<HierarchyEntry> HierarchyLinesBuilder::GetHierarchyLines()
{
  CHECK(m_getName, ());
  CHECK(m_getMainType, ());

  std::vector<HierarchyEntry> lines;
  for (auto const & tree : m_trees)
    tree_node::PreOrderVisit(tree, [&](auto const & node) { lines.emplace_back(Transform(node)); });

  return lines;
}

m2::PointD HierarchyLinesBuilder::GetCenter(HierarchyLinker::Node::Ptr const & node)
{
  auto const & data = node->GetData();
  if (!m_enricher)
    return data.GetCenter();

  auto const optCenter = m_enricher->GetFeatureCenter(data.GetCompositeId());
  return optCenter ? *optCenter : data.GetCenter();
}

HierarchyEntry HierarchyLinesBuilder::Transform(HierarchyLinker::Node::Ptr const & node)
{
  HierarchyEntry line;
  auto const & data = node->GetData();
  line.m_id = data.GetCompositeId();
  auto const parent = node->GetParent();
  if (parent)
    line.m_parentId = parent->GetData().GetCompositeId();

  line.m_country = m_countryName;
  line.m_depth = GetDepth(node);
  line.m_name = m_getName(data.GetName());
  line.m_type = m_getMainType(data.GetTypes());
  line.m_center = GetCenter(node);
  return line;
}

HierarchyLinker::Node::Ptrs BuildHierarchy(std::vector<feature::FeatureBuilder> && fbs,
                                           GetMainTypeFn const & getMainType,
                                           std::shared_ptr<FilterInterface> const & filter)
{
  base::EraseIf(fbs, [&](auto const & fb) { return !filter->IsAccepted(fb); });
  HierarchyLinker::Node::Ptrs places;
  places.reserve(fbs.size());
  base::Transform(fbs, std::back_inserter(places),
                  [](auto const & fb) { return tree_node::MakeTreeNode(HierarchyPlace(fb)); });
  auto nodes = HierarchyLinker(std::move(places)).Link();
  // We leave only the trees.
  base::EraseIf(nodes, [](auto const & node) { return node->HasParent(); });
  return nodes;
}

void AddChildrenTo(HierarchyLinker::Node::Ptrs & trees,
                   std::function<std::vector<HierarchyPlace>(CompositeId const &)> const & fn)
{
  for (auto & tree : trees)
  {
    CHECK(!tree->HasParent(), ());

    tree_node::PostOrderVisit(tree, [&](auto const & n)
    {
      auto const id = n->GetData().GetCompositeId();
      auto const & places = fn(id);
      for (auto place : places)
      {
        auto const newNode = tree_node::MakeTreeNode(std::move(place));
        tree_node::Link(newNode, n);
      }
    });
  }
}

void FlattenBuildingParts(HierarchyLinker::Node::Ptrs & trees)
{
  for (auto & tree : trees)
  {
    CHECK(!tree->HasParent(), ());

    std::vector<std::pair<hierarchy::HierarchyLinker::Node::Ptr, hierarchy::HierarchyLinker::Node::Ptr>>
        buildingPartsTrees;

    static auto const & buildingPartChecker = ftypes::IsBuildingPartChecker::Instance();
    std::function<void(hierarchy::HierarchyLinker::Node::Ptr const &)> visit;
    visit = [&](auto const & n)
    {
      if (buildingPartChecker(n->GetData().GetTypes()))
      {
        CHECK(n->HasParent(), ());
        auto building = n->GetParent();
        buildingPartsTrees.emplace_back(building, n);
        return;
      }

      CHECK(!buildingPartChecker(n->GetData().GetTypes()), ());
      for (auto const & ch : n->GetChildren())
        visit(ch);
    };

    visit(tree);

    for (auto const & buildingAndParts : buildingPartsTrees)
    {
      Unlink(buildingAndParts.second, buildingAndParts.first);
      tree_node::PostOrderVisit(buildingAndParts.second, [&](auto const & buildingPartNode)
      {
        CHECK(buildingPartChecker(buildingPartNode->GetData().GetTypes()), ());
        buildingPartNode->RemoveChildren();
        tree_node::Link(buildingPartNode, buildingAndParts.first);
      });
    }
  }
}
}  // namespace hierarchy
}  // namespace generator
