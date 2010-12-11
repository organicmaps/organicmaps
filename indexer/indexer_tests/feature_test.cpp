#include "../../testing/testing.hpp"

#include "../feature.hpp"
#include "../cell_id.hpp"

#include "../../geometry/point2d.hpp"

#include "../../base/stl_add.hpp"

namespace
{
  double Round(double x)
  {
    return static_cast<int>(x * 1000 + 0.5) / 1000.0;
  }

  struct PointAccumulator
  {
    vector<m2::PointD> m_V;

    void operator() (CoordPointT p)
    {
      m_V.push_back(m2::PointD(Round(p.first), Round(p.second)));
    }

    void operator() (m2::PointD a, m2::PointD b, m2::PointD c)
    {
      m_V.push_back(m2::PointD(Round(a.x), Round(a.y)));
      m_V.push_back(m2::PointD(Round(b.x), Round(b.y)));
      m_V.push_back(m2::PointD(Round(c.x), Round(c.y)));
    }
  };
}

UNIT_TEST(Feature_Deserialize)
{
  vector<int> a;
  a.push_back(1);
  a.push_back(2);
  FeatureBuilder builder;

  builder.AddName("name");

  vector<m2::PointD> points;
  {
    points.push_back(m2::PointD(1.0, 1.0));
    points.push_back(m2::PointD(0.25, 0.5));
    points.push_back(m2::PointD(0.25, 0.2));
    points.push_back(m2::PointD(1.0, 1.0));
    for (size_t i = 0; i < points.size(); ++i)
      builder.AddPoint(points[i]);
  }

  vector<m2::PointD> triangles;
  {
    triangles.push_back(m2::PointD(0.5, 0.5));
    triangles.push_back(m2::PointD(0.25, 0.5));
    triangles.push_back(m2::PointD(1.0, 1.0));
    for (size_t i = 0; i < triangles.size(); i += 3)
      builder.AddTriangle(triangles[i], triangles[i+1], triangles[i+2]);
  }

  builder.AddLayer(3);

  size_t const typesCount = 2;
  uint32_t arrTypes[typesCount+1] = { 5, 7, 0 };
  builder.AddTypes(arrTypes, arrTypes + typesCount);

  vector<char> serial;
  builder.Serialize(serial);
  vector<char> serial1 = serial;
  FeatureType f(serial1);

  TEST_EQUAL(f.GetFeatureType(), FeatureBase::FEATURE_TYPE_AREA, ());

  FeatureBase::GetTypesFn types;
  f.ForEachTypeRef(types);
  TEST_EQUAL(types.m_types, vector<uint32_t>(arrTypes, arrTypes + typesCount), ());

  TEST_EQUAL(f.GetLayer(), 3, ());
  TEST_EQUAL(f.GetName(), "name", ());
  TEST_EQUAL(f.GetGeometrySize(), 4, ());
  TEST_EQUAL(f.GetTriangleCount(), 1, ());

  PointAccumulator featurePoints;
  f.ForEachPointRef(featurePoints);
  TEST_EQUAL(points, featurePoints.m_V, ());

  PointAccumulator featureTriangles;
  f.ForEachTriangleRef(featureTriangles);
  TEST_EQUAL(triangles, featureTriangles.m_V, ());

  TEST_LESS(fabs(f.GetLimitRect().minX() - 0.25), 0.0001, ());
  TEST_LESS(fabs(f.GetLimitRect().minY() - 0.20), 0.0001, ());
  TEST_LESS(fabs(f.GetLimitRect().maxX() - 1.00), 0.0001, ());
  TEST_LESS(fabs(f.GetLimitRect().maxY() - 1.00), 0.0001, ());

  vector<char> serial2;
  FeatureBuilder builder2;
  f.InitFeatureBuilder(builder2);
  builder2.Serialize(serial2);

  TEST_EQUAL(serial, serial2,
             (f.DebugString(), FeatureType(serial2).DebugString()));
}
