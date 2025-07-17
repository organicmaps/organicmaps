#include "poly_borders/poly_borders_tests/tools.hpp"

#include "testing/testing.hpp"

#include "poly_borders/borders_data.hpp"

#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace platform::tests_support;
using namespace platform;
using namespace poly_borders;
using namespace std;

namespace
{
static string const kTestDir = "borders_poly_dir";

void TestMarked(Polygon const & polygon, size_t i)
{
  TEST(polygon.m_points[i].m_marked, (i, "th point point must be marked."));
}

void TestNotMarked(Polygon const & polygon, size_t i)
{
  TEST(!polygon.m_points[i].m_marked, (i, "th point must not be marked."));
}

void CheckByMask(Polygon const & polygons, vector<bool> markedMask)
{
  CHECK_EQUAL(polygons.m_points.size(), markedMask.size(), ());
  for (size_t i = 0; i < polygons.m_points.size(); ++i)
  {
    if (markedMask[i])
      TestMarked(polygons, i);
    else
      TestNotMarked(polygons, i);
  }
}

UNIT_TEST(PolyBordersPostprocessor_MarkPoints_1)
{
  ScopedDir const scopedDir(kTestDir);
  string const & bordersDir = scopedDir.GetFullPath();

  m2::PointD a(-1.0, -1.0);
  m2::PointD b(-1.0, 1.0);

  vector<vector<m2::PointD>> polygons1 = {
      {a, b, {1.0, 1.0}, {1.0, -1.0}}
  };

  vector<vector<bool>> markedMask1 = {
      {true, true, false, false}
  };

  vector<vector<m2::PointD>> polygons2 = {
      {a, b, {2.0, 1.0}, {5.0, -1.0}}
  };

  vector<vector<bool>> markedMask2 = {
      {true, true, false, false}
  };

  vector<shared_ptr<ScopedFile>> files;
  files.emplace_back(CreatePolyBorderFileByPolygon(kTestDir, "First", polygons1));
  files.emplace_back(CreatePolyBorderFileByPolygon(kTestDir, "Second", polygons2));

  BordersData bordersData;
  bordersData.Init(bordersDir);
  bordersData.MarkPoints();

  auto const & bordersPolygon1 = bordersData.GetBordersPolygonByName("First" + BordersData::kBorderExtension + "1");
  CheckByMask(bordersPolygon1, markedMask1[0]);

  auto const & bordersPolygon2 = bordersData.GetBordersPolygonByName("Second" + BordersData::kBorderExtension + "1");
  CheckByMask(bordersPolygon2, markedMask2[0]);
}

UNIT_TEST(PolyBordersPostprocessor_MarkPoints_2)
{
  ScopedDir const scopedDir(kTestDir);
  string const & bordersDir = scopedDir.GetFullPath();

  vector<vector<m2::PointD>> polygons1 = {
      {{-1.0, -1.0}, {-1.0, 1.0}, {1.0, 1.0}, {1.0, -1.0}}
  };

  vector<vector<bool>> markedMask1 = {
      {false, false, false, false}
  };

  vector<vector<m2::PointD>> polygons2 = {
      {{-12.0, -1.0}, {-10.0, 1.0}, {2.0, 1.0}, {5.0, -1.0}}
  };

  vector<vector<bool>> markedMask2 = {
      {false, false, false, false}
  };

  vector<shared_ptr<ScopedFile>> files;
  files.emplace_back(CreatePolyBorderFileByPolygon(kTestDir, "First", polygons1));
  files.emplace_back(CreatePolyBorderFileByPolygon(kTestDir, "Second", polygons2));

  BordersData bordersData;
  bordersData.Init(bordersDir);
  bordersData.MarkPoints();

  auto const & bordersPolygon1 = bordersData.GetBordersPolygonByName("First" + BordersData::kBorderExtension + "1");
  CheckByMask(bordersPolygon1, markedMask1[0]);

  auto const & bordersPolygon2 = bordersData.GetBordersPolygonByName("Second" + BordersData::kBorderExtension + "1");
  CheckByMask(bordersPolygon2, markedMask2[0]);
}

UNIT_TEST(PolyBordersPostprocessor_MarkPoints_3)
{
  ScopedDir const scopedDir(kTestDir);
  string const & bordersDir = scopedDir.GetFullPath();

  m2::PointD a(-2.0, 1.0);
  m2::PointD b(0.0, 3.0);
  m2::PointD c(3.0, -1.0);
  m2::PointD d(-1.0, -3.0);
  m2::PointD e(-4.0, 2.0);
  m2::PointD f(-1.0, 4.0);

  vector<vector<m2::PointD>> polygons1 = {
      {a, b, c, {1.0, -3.0}, d}
  };

  vector<vector<bool>> markedMask1 = {
      {true, true, true, false, true}
  };

  vector<vector<m2::PointD>> polygons2 = {
      {b, f, {2.0, 5.0}, {6.0, 3.0}, c}
  };

  vector<vector<bool>> markedMask2 = {
      {true, true, false, false, true}
  };

  vector<vector<m2::PointD>> polygons3 = {
      {a, b, f, {-3.0, 4.0}, e}
  };

  vector<vector<bool>> markedMask3 = {
      {true, true, true, false, true}
  };

  vector<vector<m2::PointD>> polygons4 = {
      {a, e, {-3.0, -1.0}, d}
  };

  vector<vector<bool>> markedMask4 = {
      {true, true, false, true}
  };

  vector<shared_ptr<ScopedFile>> files;
  files.emplace_back(CreatePolyBorderFileByPolygon(kTestDir, "First", polygons1));
  files.emplace_back(CreatePolyBorderFileByPolygon(kTestDir, "Second", polygons2));
  files.emplace_back(CreatePolyBorderFileByPolygon(kTestDir, "Third", polygons3));
  files.emplace_back(CreatePolyBorderFileByPolygon(kTestDir, "Fourth", polygons4));

  BordersData bordersData;
  bordersData.Init(bordersDir);
  bordersData.MarkPoints();

  auto const & bordersPolygon1 = bordersData.GetBordersPolygonByName("First" + BordersData::kBorderExtension + "1");
  CheckByMask(bordersPolygon1, markedMask1[0]);

  auto const & bordersPolygon2 = bordersData.GetBordersPolygonByName("Second" + BordersData::kBorderExtension + "1");
  CheckByMask(bordersPolygon2, markedMask2[0]);

  auto const & bordersPolygon3 = bordersData.GetBordersPolygonByName("Third" + BordersData::kBorderExtension + "1");
  CheckByMask(bordersPolygon3, markedMask3[0]);

  auto const & bordersPolygon4 = bordersData.GetBordersPolygonByName("Fourth" + BordersData::kBorderExtension + "1");
  CheckByMask(bordersPolygon4, markedMask4[0]);
}

UNIT_TEST(PolyBordersPostprocessor_MarkPoints_4)
{
  ScopedDir const scopedDir(kTestDir);
  string const & bordersDir = scopedDir.GetFullPath();

  m2::PointD a(6.0, 2.0);
  m2::PointD b(6.0, 4.0);
  
  vector<vector<m2::PointD>> polygons1 = {
      {{-2.0, -2.0}, {-2.0, 2.0}, {2.0, 2.0}, {2.0, -2.0}},
      {{4.0, 2.0}, {4.0, 4.0}, a, b}
  };

  vector<vector<bool>> markedMask1 = {
      {false, false, false, false},
      {false, false, true, true}
  };

  vector<vector<m2::PointD>> polygons2 = {
      {a, b, {8.0, 6.0}, {8.0, 0.0}}
  };

  vector<vector<bool>> markedMask2 = {
      {true, true, false, false}
  };

  vector<shared_ptr<ScopedFile>> files;
  files.emplace_back(CreatePolyBorderFileByPolygon(kTestDir, "First", polygons1));
  files.emplace_back(CreatePolyBorderFileByPolygon(kTestDir, "Second", polygons2));

  BordersData bordersData;
  bordersData.Init(bordersDir);
  bordersData.MarkPoints();

  auto const & firstBordersPolygon1 = bordersData.GetBordersPolygonByName("First" + BordersData::kBorderExtension + "1");
  CheckByMask(firstBordersPolygon1, markedMask1[0]);

  auto const & secondBordersPolygon1 = bordersData.GetBordersPolygonByName("First" + BordersData::kBorderExtension + "2");
  CheckByMask(secondBordersPolygon1, markedMask1[1]);

  auto const & bordersPolygon2 = bordersData.GetBordersPolygonByName("Second" + BordersData::kBorderExtension + "1");
  CheckByMask(bordersPolygon2, markedMask2[0]);
}
}  // namespace
