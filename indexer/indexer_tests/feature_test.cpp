#include "feature_routine.hpp"

#include "../../testing/testing.hpp"

#include "../feature.hpp"
#include "../cell_id.hpp"
#include "../classificator.hpp"
#include "../classificator_loader.hpp"

#include "../../platform/platform.hpp"

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
  Platform & platform = GetPlatform();
  classificator::Read(platform.ReadPathForFile("drawing_rules.bin"),
                      platform.ReadPathForFile("classificator.txt"),
                      platform.ReadPathForFile("visibility.txt"));

  FeatureBuilder2 fb;

  fb.AddName("name");

  vector<m2::PointD> points;
  {
    points.push_back(m2::PointD(1.0, 1.0));
    points.push_back(m2::PointD(0.25, 0.5));
    points.push_back(m2::PointD(0.25, 0.2));
    points.push_back(m2::PointD(1.0, 1.0));
    for (size_t i = 0; i < points.size(); ++i)
      fb.AddPoint(points[i]);
    fb.SetLinear();
  }

  //vector<m2::PointD> triangles;
  //{
  //  triangles.push_back(m2::PointD(0.5, 0.5));
  //  triangles.push_back(m2::PointD(0.25, 0.5));
  //  triangles.push_back(m2::PointD(1.0, 1.0));
  //  for (size_t i = 0; i < triangles.size(); i += 3)
  //    fb.AddTriangle(triangles[i], triangles[i+1], triangles[i+2]);
  //}

  fb.AddLayer(3);

  vector<uint32_t> types;
  {
    vector<string> path;
    path.push_back("natural");
    path.push_back("coastline");

    uint32_t const type = classif().GetTypeByPath(path);
    TEST_NOT_EQUAL(type, ftype::GetEmptyValue(), () );
    types.push_back(type);

    fb.AddTypes(types.begin(), types.end());
  }

  FeatureType f;
  FeatureBuilder2Feature(fb, f);

  TEST_EQUAL(f.GetFeatureType(), FeatureBase::FEATURE_TYPE_LINE, ());

  FeatureBase::GetTypesFn getTypes;
  f.ForEachTypeRef(getTypes);
  TEST_EQUAL(vector<uint32_t>(getTypes.m_types, getTypes.m_types + getTypes.m_size), types, ());

  TEST_EQUAL(f.GetLayer(), 3, ());
  TEST_EQUAL(f.GetName(), "name", ());
  //TEST_EQUAL(f.GetGeometrySize(), 4, ());
  //TEST_EQUAL(f.GetTriangleCount(), 1, ());

  int const level = 0;

  PointAccumulator featurePoints;
  f.ForEachPointRef(featurePoints, level);
  TEST_EQUAL(points, featurePoints.m_V, ());

  //PointAccumulator featureTriangles;
  //f.ForEachTriangleRef(featureTriangles, level);
  //TEST_EQUAL(triangles, featureTriangles.m_V, ());

  double const eps = MercatorBounds::GetCellID2PointAbsEpsilon();
  m2::RectD const & rect = f.GetLimitRect(level);

  TEST_LESS(fabs(rect.minX() - 0.25), eps, ());
  TEST_LESS(fabs(rect.minY() - 0.20), eps, ());
  TEST_LESS(fabs(rect.maxX() - 1.00), eps, ());
  TEST_LESS(fabs(rect.maxY() - 1.00), eps, ());

  //{
  //  FeatureBuilder2 fbTest;
  //  Feature2FeatureBuilder(f, fbTest);

  //  FeatureType fTest;
  //  FeatureBuilder2Feature(fbTest, fTest);
  //  TEST_EQUAL(f.DebugString(level), fTest.DebugString(level), ());
  //}
}
