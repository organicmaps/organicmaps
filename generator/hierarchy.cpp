#include "generator/hierarchy.hpp"

#include "generator/boost_helpers.hpp"
#include "generator/place_processor.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_utils.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iterator>
#include <limits>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
#include <boost/geometry/multi/geometries/register/multi_point.hpp>

BOOST_GEOMETRY_REGISTER_POINT_2D(m2::PointD, double, boost::geometry::cs::cartesian, x, y);
BOOST_GEOMETRY_REGISTER_RING(std::vector<m2::PointD>);

using namespace feature;

namespace generator
{
namespace hierarchy
{
namespace
{
// GetRussianName returns a russian feature name if it's possible.
// Otherwise, GetRussianName function returns a name that GetReadableName returns.
std::string GetRussianName(StringUtf8Multilang const & str)
{
  auto const deviceLang = StringUtf8Multilang::GetLangIndex("ru");
  std::string result;
  GetReadableName({} /* regionData */, str, deviceLang, false /* allowTranslit */, result);
  for (auto const & ch : {';', '\n', '\t'})
    std::replace(std::begin(result), std::end(result), ch, ',');
  return result;
}
}  // namespace

uint32_t GetTypeDefault(FeatureParams::Types const &) { return ftype::GetEmptyValue(); }

std::string GetNameDefault(StringUtf8Multilang const &) { return {}; }

std::string PrintDefault(HierarchyLine const &) { return {}; }

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

  return m_rect.IsRectInside(smaller.GetLimitRect()) &&
         boost::geometry::covered_by(smaller.m_polygon, m_polygon);
}

bool HierarchyPlace::Contains(m2::PointD const & point) const
{
  return boost::geometry::covered_by(point, m_polygon);
}

bool HierarchyPlace::IsEqualGeometry(HierarchyPlace const & other) const
{
  return IsPoint() ? boost::geometry::equals(m_center, other.m_center)
                   : boost::geometry::equals(m_polygon, other.m_polygon);
}

HierarchyLinker::HierarchyLinker(Node::PtrList && nodes)
  : m_nodes(std::move(nodes)), m_tree(MakeTree4d(m_nodes))
{
}

// static
HierarchyLinker::Tree4d HierarchyLinker::MakeTree4d(Node::PtrList const & nodes)
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
  m_tree.ForEachInRect({point, point}, [&](auto const & candidatePtr) {
    auto const & candidate = candidatePtr->GetData();
    if (place.GetCompositeId() == candidate.GetCompositeId())
      return;
    // Sometimes there can be two places with the same geometry. We must compare their ids
    // to avoid cyclic connections.
    if (place.IsEqualGeometry(candidate))
    {
      if (place.GetCompositeId() < candidate.GetCompositeId())
        parent = candidatePtr;
    }
    else if (candidate.GetArea() < minArea && candidate.Contains(place))
    {
      parent = candidatePtr;
      minArea = candidate.GetArea();
    }
  });
  return parent;
}

HierarchyLinker::Node::PtrList HierarchyLinker::Link()
{
  for (auto & node : m_nodes)
  {
    auto const & place = node->GetData();
    auto const parentPlace = FindPlaceParent(place);
    if (!parentPlace)
      continue;

    parentPlace->AddChild(node);
    node->SetParent(parentPlace);
  }
  return m_nodes;
}

HierarchyBuilder::HierarchyBuilder(std::string const & dataFilename)
  : m_dataFullFilename(dataFilename)
{
}

void HierarchyBuilder::SetGetMainTypeFunction(GetMainType const & getMainType)
{
  m_getMainType = getMainType;
}

void HierarchyBuilder::SetGetNameFunction(GetName const & getName) { m_getName = getName; }

std::vector<feature::FeatureBuilder> HierarchyBuilder::ReadFeatures(
    std::string const & dataFilename)
{
  std::vector<feature::FeatureBuilder> fbs;
  ForEachFromDatRawFormat<serialization_policy::MaxAccuracy>(
      dataFilename, [&](FeatureBuilder const & fb, uint64_t /* currPos */) {
        if (m_getMainType(fb.GetTypes()) != ftype::GetEmptyValue() &&
            !m_getName(fb.GetMultilangName()).empty() && !fb.GetOsmIds().empty() &&
            (fb.IsPoint() || fb.IsArea()))
        {
          fbs.emplace_back(fb);
        }
      });
  return fbs;
}

HierarchyBuilder::Node::PtrList HierarchyBuilder::Build()
{
  auto const fbs = ReadFeatures(m_dataFullFilename);
  Node::PtrList places;
  places.reserve(fbs.size());
  std::transform(std::cbegin(fbs), std::cend(fbs), std::back_inserter(places),
                 [](auto const & fb) { return std::make_shared<Node>(HierarchyPlace(fb)); });
  return HierarchyLinker(std::move(places)).Link();
}

HierarchyLineEnricher::HierarchyLineEnricher(std::string const & osm2FtIdsPath,
                                             std::string const & countryFullPath)
  : m_featureGetter(countryFullPath)
{
  CHECK(m_osm2FtIds.ReadFromFile(osm2FtIdsPath), (osm2FtIdsPath));
}

boost::optional<m2::PointD> HierarchyLineEnricher::GetFeatureCenter(CompositeId const & id) const
{
  auto const optId = m_osm2FtIds.GetFeatureId(id);
  if (!optId)
    return {};

  auto const ftPtr = m_featureGetter.GetFeatureByIndex(*optId);
  return ftPtr ? feature::GetCenter(*ftPtr) : boost::optional<m2::PointD>();
}

std::string DebugPrint(HierarchyLine const & line)
{
  std::stringstream stream;
  stream << std::fixed << std::setprecision(7);
  stream << DebugPrint(line.m_id) << ';';
  if (line.m_parentId)
    stream << DebugPrint(*line.m_parentId);
  stream << ';';
  stream << line.m_depth << ';';
  stream << line.m_center.x << ';';
  stream << line.m_center.y << ';';
  stream << classif().GetReadableObjectName(line.m_type) << ';';
  stream << line.m_name << ';';
  stream << line.m_countryName;
  return stream.str();
}

HierarchyLinesBuilder::HierarchyLinesBuilder(HierarchyBuilder::Node::PtrList && nodes)
  : m_nodes(std::move(nodes))
{
}

void HierarchyLinesBuilder::SetGetMainTypeFunction(GetMainType const & getMainType)
{
  m_getMainType = getMainType;
}

void HierarchyLinesBuilder::SetGetNameFunction(GetName const & getName) { m_getName = getName; }

void HierarchyLinesBuilder::SetCountryName(std::string const & name) { m_countryName = name; }

void HierarchyLinesBuilder::SetHierarchyLineEnricher(
    std::shared_ptr<HierarchyLineEnricher> const & enricher)
{
  m_enricher = enricher;
}

std::vector<HierarchyLine> HierarchyLinesBuilder::GetHierarchyLines()
{
  std::vector<HierarchyLine> lines;
  lines.reserve(m_nodes.size());
  std::transform(std::cbegin(m_nodes), std::cend(m_nodes), std::back_inserter(lines),
                 [&](auto const & n) { return Transform(n); });
  return lines;
}

m2::PointD HierarchyLinesBuilder::GetCenter(HierarchyBuilder::Node::Ptr const & node)
{
  auto const & data = node->GetData();
  if (!m_enricher)
    return data.GetCenter();

  auto const optCenter = m_enricher->GetFeatureCenter(data.GetCompositeId());
  return optCenter ? *optCenter : data.GetCenter();
}

HierarchyLine HierarchyLinesBuilder::Transform(HierarchyBuilder::Node::Ptr const & node)
{
  HierarchyLine line;
  auto const & data = node->GetData();
  line.m_id = data.GetCompositeId();
  auto const parent = node->GetParent();
  if (parent)
    line.m_parentId = parent->GetData().GetCompositeId();

  line.m_countryName = m_countryName;
  line.m_depth = GetDepth(node);
  line.m_name = m_getName(data.GetName());
  line.m_type = m_getMainType(data.GetTypes());
  line.m_center = GetCenter(node);
  return line;
}

namespace popularity
{
uint32_t GetMainType(FeatureParams::Types const & types)
{
  auto const & airportChecker = ftypes::IsAirportChecker::Instance();
  auto it = base::FindIf(types, airportChecker);
  if (it != std::cend(types))
    return *it;

  auto const & attractChecker = ftypes::AttractionsChecker::Instance();
  auto const type = attractChecker.GetBestType(types);
  if (type != ftype::GetEmptyValue())
    return type;

  auto const & eatChecker = ftypes::IsEatChecker::Instance();
  it = base::FindIf(types, eatChecker);
  return it != std::cend(types) ? *it : ftype::GetEmptyValue();
}

std::string GetName(StringUtf8Multilang const & str) { return GetRussianName(str); }

std::string Print(HierarchyLine const & line)
{
  std::stringstream stream;
  stream << std::fixed << std::setprecision(7);
  stream << line.m_id.m_mainId.GetEncodedId() << '|' << line.m_id.m_additionalId.GetEncodedId()
         << ';';
  if (line.m_parentId)
  {
    auto const parentId = *line.m_parentId;
    stream << parentId.m_mainId.GetEncodedId() << '|' << parentId.m_additionalId.GetEncodedId()
           << ';';
  }
  stream << ';';
  stream << line.m_center.x << ';';
  stream << line.m_center.y << ';';
  stream << classif().GetReadableObjectName(line.m_type) << ';';
  stream << line.m_name;
  return stream.str();
}

}  // namespace popularity
}  // namespace hierarchy
}  // namespace generator
