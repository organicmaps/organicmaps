#include "testing/testing.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/index_graph.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/map.hpp"
#include "std/vector.hpp"

namespace
{
using namespace routing;

class TestGeometry : public Geometry
{
public:
  // Geometry overrides:
  bool IsRoad(uint32_t featureId) const override;
  bool IsOneWay(uint32_t featureId) const override;
  m2::PointD const & GetPoint(FSegId fseg) const override;
  double CalcEdgesWeight(uint32_t featureId, uint32_t pointStart,
                         uint32_t pointFinish) const override;
  double CalcHeuristic(FSegId from, FSegId to) const override;

  void AddRoad(uint32_t featureId, vector<m2::PointD> const & points);

private:
  map<uint32_t, vector<m2::PointD>> m_features;
};

bool TestGeometry::IsRoad(uint32_t featureId) const
{
  return m_features.find(featureId) != m_features.end();
}

bool TestGeometry::IsOneWay(uint32_t featureId) const { return false; }

m2::PointD const & TestGeometry::GetPoint(FSegId fseg) const
{
  auto it = m_features.find(fseg.GetFeatureId());
  if (it == m_features.end())
  {
    ASSERT(false, ("Can't find feature", fseg.GetFeatureId()));
    static m2::PointD invalidResult(-1.0, -1.0);
    return invalidResult;
  }

  ASSERT_LESS(fseg.GetSegId(), it->second.size(), ("featureId =", fseg.GetFeatureId()));
  return it->second[fseg.GetSegId()];
}

double TestGeometry::CalcEdgesWeight(uint32_t featureId, uint32_t pointFrom, uint32_t pointTo) const
{
  auto it = m_features.find(featureId);
  if (it == m_features.end())
  {
    ASSERT(false, ("Can't find feature", featureId));
    return 0.0;
  }
  vector<m2::PointD> const & points = it->second;

  uint32_t const start = min(pointFrom, pointTo);
  uint32_t const finish = max(pointFrom, pointTo);
  ASSERT_LESS(finish, points.size(), ());

  double result = 0.0;
  for (uint32_t i = start; i < finish; ++i)
    result += points[i].Length(points[i + 1]);

  return result;
}

double TestGeometry::CalcHeuristic(FSegId from, FSegId to) const
{
  return GetPoint(from).Length(GetPoint(to));
}

void TestGeometry::AddRoad(uint32_t featureId, vector<m2::PointD> const & points)
{
  auto it = m_features.find(featureId);
  if (it != m_features.end())
  {
    ASSERT(false, ("Already contains feature", featureId));
    return;
  }

  m_features[featureId] = points;
}

Joint MakeJoint(vector<FSegId> const & points)
{
  Joint joint;
  for (auto const & point : points)
  {
    joint.AddEntry(point);
  }
  return joint;
}

void CheckRoute(IndexGraph & graph, FSegId const & start, FSegId const & finish,
                size_t expectedLength)
{
  LOG(LINFO, ("Check route", start.GetFeatureId(), ",", start.GetSegId(), "=>",
              finish.GetFeatureId(), ",", finish.GetSegId()));

  AStarAlgorithm<IndexGraph> algorithm;
  RoutingResult<JointId> routingResult;

  AStarAlgorithm<IndexGraph>::Result const resultCode = algorithm.FindPath(
      graph, graph.InsertJoint(start), graph.InsertJoint(finish), routingResult, {}, {});
  vector<FSegId> const & fsegs = graph.RedressRoute(routingResult.path);

  TEST_EQUAL(resultCode, AStarAlgorithm<IndexGraph>::Result::OK, ());
  TEST_EQUAL(fsegs.size(), expectedLength, ());
}

uint32_t AbsDelta(uint32_t v0, uint32_t v1) { return v0 > v1 ? v0 - v1 : v1 - v0; }
}  // namespace

namespace routing_test
{
//            R1:
//
//            -2
//            -1
//  R0: -2 -1  0  1  2
//             1
//             2
//
UNIT_TEST(FindPathCross)
{
  unique_ptr<TestGeometry> geometry = make_unique<TestGeometry>();
  geometry->AddRoad(0, {{-2.0, 0.0}, {-1.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}});
  geometry->AddRoad(1, {{0.0, -2.0}, {-1.0, 0.0}, {0.0, 0.0}, {0.0, 1.0}, {0.0, 2.0}});

  IndexGraph graph(move(geometry));

  graph.Export({MakeJoint({{0, 2}, {1, 2}})});

  vector<FSegId> points;
  for (uint32_t i = 0; i < 5; ++i)
  {
    points.emplace_back(0, i);
    points.emplace_back(1, i);
  }

  for (auto const & start : points)
  {
    for (auto const & finish : points)
    {
      uint32_t expectedLength;
      if (start.GetFeatureId() == finish.GetFeatureId())
        expectedLength = AbsDelta(start.GetSegId(), finish.GetSegId()) + 1;
      else
        expectedLength = AbsDelta(start.GetSegId(), 2) + AbsDelta(finish.GetSegId(), 2) + 1;
      CheckRoute(graph, start, finish, expectedLength);
    }
  }
}

//      R4  R5  R6  R7
//
//  R0:  0 - * - * - *
//       |   |   |   |
//  R1:  * - 1 - * - *
//       |   |   |   |
//  R2   * - * - 2 - *
//       |   |   |   |
//  R3   * - * - * - 3
//
UNIT_TEST(FindPathManhattan)
{
  uint32_t constexpr kCitySize = 4;
  unique_ptr<TestGeometry> geometry = make_unique<TestGeometry>();
  for (uint32_t i = 0; i < kCitySize; ++i)
  {
    vector<m2::PointD> horizontalRoad;
    vector<m2::PointD> verticalRoad;
    for (uint32_t j = 0; j < kCitySize; ++j)
    {
      horizontalRoad.emplace_back((double)i, (double)j);
      verticalRoad.emplace_back((double)j, (double)i);
    }
    geometry->AddRoad(i, horizontalRoad);
    geometry->AddRoad(i + kCitySize, verticalRoad);
  }

  IndexGraph graph(move(geometry));

  vector<Joint> joints;
  for (uint32_t i = 0; i < kCitySize; ++i)
  {
    for (uint32_t j = 0; j < kCitySize; ++j)
    {
      joints.emplace_back(MakeJoint({{i, j}, {j + kCitySize, i}}));
    }
  }
  graph.Export(joints);

  for (uint32_t startY = 0; startY < kCitySize; ++startY)
  {
    for (uint32_t startX = 0; startX < kCitySize; ++startX)
    {
      for (uint32_t finishY = 0; finishY < kCitySize; ++finishY)
      {
        for (uint32_t finishX = 0; finishX < kCitySize; ++finishX)
        {
          CheckRoute(graph, {startX, startY}, {finishX, finishY},
                     AbsDelta(startX, finishX) + AbsDelta(startY, finishY) + 1);
        }
      }
    }
  }
}
}  // namespace routing_test
