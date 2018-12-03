#pragma once

#include "generator/feature_builder.hpp"
#include "generator/place_node.hpp"

#include "geometry/point2d.hpp"
#include "geometry/tree4d.hpp"

#include "base/geo_object_id.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/geometry.hpp>
#include <boost/optional.hpp>

namespace generator_tests
{
class TestPopularityBuilder;
}  // namespace generator_tests

namespace generator
{
namespace popularity
{
// These are functions for generating a csv file for popularity.
// dataFilename - A path to data file
// dataDir - A path to the directory where the data files are located
// dataFilenames - Paths to data files
// cpuCount - A number of processes
// outFilename - A path where the csv file will be saved

// Csv format:
// Id;Parent id;Lat;Lon;Main type;Name
// 9223372036936022489;;42.996411;41.004747;leisure-park;Сквер им. И. А. Когония
// 9223372037297546235;;43.325002;40.224941;leisure-park;Приморский парк
// 9223372036933918763;;43.005177;41.022295;leisure-park;Сухумский ботанический сад
void BuildPopularitySrcFromData(std::string const & dataFilename, std::string const & outFilename);

void BuildPopularitySrcFromAllData(std::string const & dataDir, std::string const & outFilename,
                                   size_t cpuCount = 1);

void BuildPopularitySrcFromAllData(std::vector<std::string> const & dataFilenames, std::string const & outFilename,
                                   size_t cpuCount = 1);

class PopularityGeomPlace
{
public:
  explicit PopularityGeomPlace(FeatureBuilder1 const & feature);

  bool Contains(PopularityGeomPlace const & smaller) const;
  bool Contains(m2::PointD const & point) const;
  FeatureBuilder1 const & GetFeature() const { return m_feature; }
  void DeletePolygon() { m_polygon = nullptr; }
  double GetArea() const { return m_area; }
  base::GeoObjectId GetId() const { return m_id; }

private:
  using BoostPoint = boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>;
  using BoostPolygon = boost::geometry::model::polygon<BoostPoint>;

  base::GeoObjectId m_id;
  std::reference_wrapper<FeatureBuilder1 const> m_feature;
  std::unique_ptr<BoostPolygon> m_polygon;
  double m_area;
};

struct PopularityLine
{
  base::GeoObjectId m_id;
  boost::optional<base::GeoObjectId> m_parent;
  m2::PointD m_center;
  std::string m_type;
  std::string m_name;
};

class PopularityBuilder
{
public:
  friend class generator_tests::TestPopularityBuilder;

  explicit PopularityBuilder(std::string const & dataFilename);

  std::vector<PopularityLine> Build() const;

private:
  using Node = PlaceNode<PopularityGeomPlace>;
  using Tree4d = m4::Tree<base::GeoObjectId>;
  using MapIdToNode = std::unordered_map<base::GeoObjectId, Node::Ptr>;

  static std::string GetType(FeatureBuilder1 const & feature);
  static std::string GetFeatureName(FeatureBuilder1 const & feature);
  static void FillLinesFromPointObjects(std::vector<FeatureBuilder1> const & pointObjs, MapIdToNode const & m,
                                        Tree4d const & tree, std::vector<PopularityLine> & lines);
  static boost::optional<base::GeoObjectId>
  FindPointParent(m2::PointD const & point, MapIdToNode const & m, Tree4d const & tree);
  static boost::optional<Node::Ptr>
  FindPopularityGeomPlaceParent(PopularityGeomPlace const & place, MapIdToNode const & m,
                                Tree4d const & tree);
  static MapIdToNode GetAreaMap(Node::PtrList const & nodes);
  static Tree4d MakeTree4d(Node::PtrList const & nodes);
  static void FillLineFromGeomObjectPtr(PopularityLine & line, Node::Ptr const & node);
  static void FillLinesFromGeomObjectPtrs(Node::PtrList const & nodes,
                                          std::vector<PopularityLine> & lines);
  static void LinkGeomPlaces(MapIdToNode const & m, Tree4d const & tree, Node::PtrList & nodes);
  static Node::PtrList MakeNodes(std::vector<FeatureBuilder1> const & features);

  std::string m_dataFilename;
};
}  // namespace popularity
}  // namespace generator
