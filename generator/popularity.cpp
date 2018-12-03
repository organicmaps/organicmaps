#include "generator/popularity.hpp"

#include "generator/boost_helpers.hpp"
#include "generator/platform_helpers.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"

#include <algorithm>
#include <functional>
#include <fstream>
#include <limits>
#include <memory>

#include "3party/ThreadPool/ThreadPool.h"

namespace generator
{
namespace popularity
{
PopularityGeomPlace::PopularityGeomPlace(FeatureBuilder1 const & feature)
  : m_id(feature.GetMostGenericOsmId())
  , m_feature(feature)
  , m_polygon(std::make_unique<BoostPolygon>())
{
  CHECK(m_polygon, ());
  boost_helpers::FillPolygon(*m_polygon, m_feature);
  m_area = boost::geometry::area(*m_polygon);
}

bool PopularityGeomPlace::Contains(PopularityGeomPlace const & smaller) const
{
  CHECK(m_polygon, ());
  CHECK(smaller.m_polygon, ());

  return GetFeature().GetLimitRect().IsRectInside(smaller.GetFeature().GetLimitRect()) &&
      boost::geometry::covered_by(*smaller.m_polygon, *m_polygon);
}

bool PopularityGeomPlace::Contains(m2::PointD const & point) const
{
  CHECK(m_polygon, ());

  return GetFeature().GetLimitRect().IsPointInside(point) &&
      boost::geometry::covered_by(BoostPoint(point.x, point.y), *m_polygon);
}

PopularityBuilder::PopularityBuilder(std::string const & dataFilename)
  : m_dataFilename(dataFilename) {}

std::vector<PopularityLine> PopularityBuilder::Build() const
{
  std::vector<FeatureBuilder1> pointObjs;
  std::vector<FeatureBuilder1> geomObjs;
  auto const & checker = ftypes::IsPopularityPlaceChecker::Instance();
  feature::ForEachFromDatRawFormat(m_dataFilename, [&](FeatureBuilder1 const & fb, uint64_t /* currPos */) {
    if (!checker(fb.GetTypesHolder()) || GetFeatureName(fb).empty())
      return;

    if (fb.IsPoint())
      pointObjs.push_back(fb);
    else if (fb.IsArea() && fb.IsGeometryClosed())
      geomObjs.push_back(fb);
  });

  auto geomObjsPtrs = MakeNodes(geomObjs);
  auto const tree = MakeTree4d(geomObjsPtrs);
  auto const mapIdToNode = GetAreaMap(geomObjsPtrs);
  LinkGeomPlaces(mapIdToNode, tree, geomObjsPtrs);

  std::vector<PopularityLine> result;
  FillLinesFromGeomObjectPtrs(geomObjsPtrs, result);
  FillLinesFromPointObjects(pointObjs, mapIdToNode, tree, result);
  return result;
}

// static
std::string PopularityBuilder::GetType(FeatureBuilder1 const & feature)
{
  auto const & c = classif();
  auto const & checker = ftypes::IsPopularityPlaceChecker::Instance();
  auto const & types = feature.GetTypes();
  auto const it = std::find_if(std::begin(types), std::end(types), checker);
  return it == std::end(types) ? string() : c.GetReadableObjectName(*it);
}

// static
std::string PopularityBuilder::GetFeatureName(FeatureBuilder1 const & feature)
{
  auto const & str = feature.GetParams().name;
  auto const deviceLang = StringUtf8Multilang::GetLangIndex("ru");
  std::string result;
  feature::GetReadableName({}, str, deviceLang, false /* allowTranslit */, result);
  std::replace(std::begin(result), std::end(result), ';', ',');
  std::replace(std::begin(result), std::end(result), '\n', ',');
  return result;
}

// static
void PopularityBuilder::FillLinesFromPointObjects(std::vector<FeatureBuilder1> const & pointObjs,
                                                  MapIdToNode const & m, Tree4d const & tree,
                                                  std::vector<PopularityLine> & lines)
{
  lines.reserve(lines.size() + pointObjs.size());
  for (auto const & p : pointObjs)
  {
    auto const center = p.GetKeyPoint();
    PopularityLine line;
    line.m_id = p.GetMostGenericOsmId();
    line.m_parent = FindPointParent(center, m, tree);
    line.m_center = center;
    line.m_type = GetType(p);
    line.m_name = GetFeatureName(p);
    lines.push_back(line);
  }
}

// static
boost::optional<base::GeoObjectId>
PopularityBuilder::FindPointParent(m2::PointD const & point, MapIdToNode const & m, Tree4d const & tree)
{
  boost::optional<base::GeoObjectId> bestId;
  auto minArea = std::numeric_limits<double>::max();
  tree.ForEachInRect({point, point}, [&](base::GeoObjectId const & id) {
    if (m.count(id) == 0)
      return;

    auto const & r = m.at(id)->GetData();
    if (r.GetArea() < minArea && r.Contains(point))
    {
      minArea = r.GetArea();
      bestId = id;
    }
  });

  return bestId;
}

// static
boost::optional<PopularityBuilder::Node::Ptr>
PopularityBuilder::FindPopularityGeomPlaceParent(PopularityGeomPlace const & place,
                                                 MapIdToNode const & m, Tree4d const & tree)
{
  boost::optional<Node::Ptr> bestPlace;
  auto minArea = std::numeric_limits<double>::max();
  auto const point = place.GetFeature().GetKeyPoint();
  tree.ForEachInRect({point, point}, [&](base::GeoObjectId const & id) {
    if (m.count(id) == 0)
      return;

    auto const & r = m.at(id)->GetData();
    if (r.GetId() == place.GetId() || r.GetArea() < place.GetArea())
      return;

    if (r.GetArea() < minArea && r.Contains(place))
    {
      minArea = r.GetArea();
      bestPlace = m.at(id);
    }
  });

  return bestPlace;
}

// static
PopularityBuilder::MapIdToNode PopularityBuilder::GetAreaMap(Node::PtrList const & nodes)
{
  std::unordered_map<base::GeoObjectId, Node::Ptr> result;
  result.reserve(nodes.size());
  for (auto const & n : nodes)
  {
    auto const & d = n->GetData();
    result.emplace(d.GetId(), n);
  }

  return result;
}

// static
PopularityBuilder::Tree4d PopularityBuilder::MakeTree4d(Node::PtrList const & nodes)
{
  Tree4d tree;
  for (auto const & n : nodes)
  {
    auto const & data = n->GetData();
    auto const & feature = data.GetFeature();
    tree.Add(data.GetId(), feature.GetLimitRect());
  }

  return tree;
}

// static
void PopularityBuilder::FillLineFromGeomObjectPtr(PopularityLine & line, Node::Ptr const & node)
{
  auto const & data = node->GetData();
  auto const & feature = data.GetFeature();
  line.m_id = data.GetId();
  if (node->HasParent())
    line.m_parent = node->GetParent()->GetData().GetId();

  line.m_center = feature.GetKeyPoint();
  line.m_type = GetType(feature);
  line.m_name = GetFeatureName(feature);
}

// static
void PopularityBuilder::FillLinesFromGeomObjectPtrs(Node::PtrList const & nodes,
                                                    std::vector<PopularityLine> & lines)
{
  lines.reserve(lines.size() + nodes.size());
  for (auto const & n : nodes)
  {
    PopularityLine line;
    FillLineFromGeomObjectPtr(line, n);
    lines.push_back(line);
  }
}

// static
void PopularityBuilder::LinkGeomPlaces(MapIdToNode const & m, Tree4d const & tree, Node::PtrList & nodes)
{
  if (nodes.size() < 2)
    return;

  std::sort(std::begin(nodes), std::end(nodes), [](Node::Ptr const & l, Node::Ptr const & r) {
    return l->GetData().GetArea() < r->GetData().GetArea();
  });

  for (auto & node : nodes)
  {
    auto const & place = node->GetData();
    auto const parentPlace = FindPopularityGeomPlaceParent(place, m, tree);
    if (!parentPlace)
      continue;

    (*parentPlace)->AddChild(node);
    node->SetParent(*parentPlace);
  }
}

// static
PopularityBuilder::Node::PtrList
PopularityBuilder::MakeNodes(std::vector<FeatureBuilder1> const & features)
{
  Node::PtrList nodes;
  nodes.reserve(features.size());
  std::transform(std::begin(features), std::end(features), std::back_inserter(nodes), [](FeatureBuilder1 const & f) {
    return std::make_shared<Node>(PopularityGeomPlace(f));
  });

  return nodes;
}

std::vector<PopularityLine> BuildPopularitySrcFromData(std::string const & dataFilename)
{
  PopularityBuilder builder(dataFilename);
  return builder.Build();
}

std::vector<PopularityLine> BuildPopularitySrcFromAllData(std::vector<std::string> const & dataFilenames,
                                                          size_t cpuCount)
{
  CHECK_GREATER(cpuCount, 0, ());

  ThreadPool threadPool(cpuCount);
  std::vector<std::future<std::vector<PopularityLine>>> futures;
  for (auto const & filename : dataFilenames)
  {
    auto result = threadPool.enqueue(
                    static_cast<std::vector<PopularityLine>(*)(std::string const &)>(BuildPopularitySrcFromData),
                    filename);
    futures.emplace_back(std::move(result));
  }

  std::vector<PopularityLine> result;
  for (auto & f : futures)
  {
    auto lines = f.get();
    std::move(std::begin(lines), std::end(lines), std::back_inserter(result));
  }

  return result;
}

void WriteLines(std::vector<PopularityLine> const & lines, std::string const & outFilename)
{
  std::ofstream stream;
  stream.exceptions(std::fstream::failbit | std::fstream::badbit);
  stream.open(outFilename);
  stream << std::fixed << std::setprecision(7);
  stream << "Id;Parent id;Lat;Lon;Main type;Name\n";
  for (auto const & line : lines)
  {
    stream << line.m_id.GetEncodedId() << ";";
    if (line.m_parent)
      stream << *line.m_parent;

    auto const center = MercatorBounds::ToLatLon(line.m_center);
    stream << ";" << center.lat << ";" << center.lon << ";"
           << line.m_type << ";" << line.m_name << "\n";
  }
}

void BuildPopularitySrcFromData(std::string const & dataFilename, std::string const & outFilename)
{
  auto const lines = BuildPopularitySrcFromData(dataFilename);
  WriteLines(lines, outFilename);
}

void BuildPopularitySrcFromAllData(std::string const & dataDir, std::string const & outFilename,
                                   size_t cpuCount)
{
  auto const filenames = platform_helpers::GetFullDataTmpFilePaths(dataDir);
  auto const lines = BuildPopularitySrcFromAllData(filenames, cpuCount);
  WriteLines(lines, outFilename);
}
}  // namespace popularity
}  // namespace generator
