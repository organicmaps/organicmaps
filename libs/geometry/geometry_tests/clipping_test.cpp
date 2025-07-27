#include "testing/testing.hpp"

#include "geometry/clipping.hpp"

#include <cstddef>
#include <utility>
#include <vector>

namespace clipping_test
{
using namespace std;

bool CompareTriangleLists(vector<m2::PointD> const & list1, vector<m2::PointD> const & list2)
{
  if (list1.size() != list2.size())
    return false;

  double const kEps = 1e-5;
  for (size_t i = 0; i < list1.size(); i++)
    if (!list1[i].EqualDxDy(list2[i], kEps))
      return false;

  return true;
}

bool CompareSplineLists(vector<m2::SharedSpline> const & list1, vector<m2::SharedSpline> const & list2)
{
  if (list1.size() != list2.size())
    return false;

  double const kEps = 1e-5;
  for (size_t i = 0; i < list1.size(); i++)
  {
    auto & path1 = list1[i]->GetPath();
    auto & path2 = list2[i]->GetPath();
    if (path1.size() != path2.size())
      return false;

    for (size_t j = 0; j < path1.size(); j++)
      if (!path1[j].EqualDxDy(path2[j], kEps))
        return false;
  }

  return true;
}

vector<m2::SharedSpline> ConstructSplineList(vector<vector<m2::PointD>> const & segments)
{
  vector<m2::SharedSpline> result;
  result.reserve(segments.size());
  for (size_t i = 0; i < segments.size(); i++)
  {
    m2::SharedSpline s;
    s.Reset(new m2::Spline(segments[i].size()));
    for (size_t j = 0; j < segments[i].size(); j++)
      s->AddPoint(segments[i][j]);
    result.push_back(std::move(s));
  }
  return result;
}

UNIT_TEST(Clipping_ClipTriangleByRect)
{
  m2::RectD r(-1.0, -1.0, 1.0, 1.0);

  // Completely inside.
  vector<m2::PointD> result1;
  m2::ClipTriangleByRect(r, m2::PointD(0.5, 0.5), m2::PointD(0.5, -0.5), m2::PointD(0.0, 0.0),
                         [&result1](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
  {
    result1.push_back(p1);
    result1.push_back(p2);
    result1.push_back(p3);
  });
  vector<m2::PointD> expectedResult1 = {m2::PointD(0.5, 0.5), m2::PointD(0.5, -0.5), m2::PointD(0.0, 0.0)};
  TEST(CompareTriangleLists(result1, expectedResult1), (result1, expectedResult1));

  // 1 point inside.
  vector<m2::PointD> result2;
  m2::ClipTriangleByRect(r, m2::PointD(0.0, 0.0), m2::PointD(2.0, 2.0), m2::PointD(2.0, -2.0),
                         [&result2](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
  {
    result2.push_back(p1);
    result2.push_back(p2);
    result2.push_back(p3);
  });
  vector<m2::PointD> expectedResult2 = {m2::PointD(0.0, 0.0), m2::PointD(1.0, 1.0), m2::PointD(1.0, -1.0)};
  TEST(CompareTriangleLists(result2, expectedResult2), (result2, expectedResult2));

  // 2 points inside.
  vector<m2::PointD> result3;
  m2::ClipTriangleByRect(r, m2::PointD(0.0, 0.5), m2::PointD(2.0, 0.0), m2::PointD(0.0, -0.5),
                         [&result3](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
  {
    result3.push_back(p1);
    result3.push_back(p2);
    result3.push_back(p3);
  });
  vector<m2::PointD> expectedResult3 = {m2::PointD(0.0, 0.5), m2::PointD(1.0, 0.25),  m2::PointD(1.0, -0.25),
                                        m2::PointD(0.0, 0.5), m2::PointD(1.0, -0.25), m2::PointD(0.0, -0.5)};
  TEST(CompareTriangleLists(result3, expectedResult3), (result3, expectedResult3));

  // 2 edges clipping.
  vector<m2::PointD> result4;
  m2::ClipTriangleByRect(r, m2::PointD(0.0, 0.0), m2::PointD(0.0, 1.5), m2::PointD(1.5, 0.0),
                         [&result4](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
  {
    result4.push_back(p1);
    result4.push_back(p2);
    result4.push_back(p3);
  });
  vector<m2::PointD> expectedResult4 = {m2::PointD(0.0, 0.0), m2::PointD(0.0, 1.0), m2::PointD(0.5, 1.0),
                                        m2::PointD(0.0, 0.0), m2::PointD(0.5, 1.0), m2::PointD(1.0, 0.5),
                                        m2::PointD(0.0, 0.0), m2::PointD(1.0, 0.5), m2::PointD(1.0, 0.0)};
  TEST(CompareTriangleLists(result4, expectedResult4), (result4, expectedResult4));

  // 3 edges clipping.
  vector<m2::PointD> result5;
  m2::ClipTriangleByRect(r, m2::PointD(-1.5, 0.0), m2::PointD(0.0, 1.5), m2::PointD(1.5, 0.0),
                         [&result5](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
  {
    result5.push_back(p1);
    result5.push_back(p2);
    result5.push_back(p3);
  });
  vector<m2::PointD> expectedResult5 = {m2::PointD(-1.0, 0.5), m2::PointD(-0.5, 1.0), m2::PointD(0.5, 1.0),
                                        m2::PointD(-1.0, 0.5), m2::PointD(0.5, 1.0),  m2::PointD(1.0, 0.5),
                                        m2::PointD(-1.0, 0.5), m2::PointD(1.0, 0.5),  m2::PointD(1.0, 0.0),
                                        m2::PointD(-1.0, 0.5), m2::PointD(1.0, 0.0),  m2::PointD(-1.0, 0.0)};
  TEST(CompareTriangleLists(result5, expectedResult5), (result5, expectedResult5));

  // Completely outside.
  vector<m2::PointD> result6;
  m2::ClipTriangleByRect(r, m2::PointD(1.5, 1.5), m2::PointD(1.5, -1.5), m2::PointD(2.0, 0.0),
                         [&result6](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
  {
    result6.push_back(p1);
    result6.push_back(p2);
    result6.push_back(p3);
  });
  vector<m2::PointD> expectedResult6 = {};
  TEST(CompareTriangleLists(result6, expectedResult6), (result6, expectedResult6));

  // Clip with an angle of rect.
  vector<m2::PointD> result7;
  m2::ClipTriangleByRect(r, m2::PointD(0.5, 0.5), m2::PointD(0.5, 2.0), m2::PointD(2.0, 0.5),
                         [&result7](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
  {
    result7.push_back(p1);
    result7.push_back(p2);
    result7.push_back(p3);
  });
  vector<m2::PointD> expectedResult7 = {m2::PointD(0.5, 0.5), m2::PointD(0.5, 1.0), m2::PointD(1.0, 1.0),
                                        m2::PointD(0.5, 0.5), m2::PointD(1.0, 1.0), m2::PointD(1.0, 0.5)};
  TEST(CompareTriangleLists(result7, expectedResult7), (result7, expectedResult7));

  // Triangle covers rect.
  vector<m2::PointD> result8;
  m2::ClipTriangleByRect(r, m2::PointD(0.0, 3.0), m2::PointD(5.0, -2.0), m2::PointD(-5.0, -2.0),
                         [&result8](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
  {
    result8.push_back(p1);
    result8.push_back(p2);
    result8.push_back(p3);
  });
  vector<m2::PointD> expectedResult8 = {m2::PointD(-1.0, 1.0), m2::PointD(1.0, 1.0),   m2::PointD(1.0, -1.0),
                                        m2::PointD(1.0, -1.0), m2::PointD(-1.0, -1.0), m2::PointD(-1.0, 1.0)};
  TEST(CompareTriangleLists(result8, expectedResult8), (result8, expectedResult8));

  // Clip with an angle of rect.
  vector<m2::PointD> result9;
  m2::ClipTriangleByRect(r, m2::PointD(1.5, 0.0), m2::PointD(1.5, -1.5), m2::PointD(0.0, -1.5),
                         [&result9](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
  {
    result9.push_back(p1);
    result9.push_back(p2);
    result9.push_back(p3);
  });
  vector<m2::PointD> expectedResult9 = {m2::PointD(0.5, -1.0), m2::PointD(1.0, -0.5), m2::PointD(1.0, -1.0)};
  TEST(CompareTriangleLists(result9, expectedResult9), (result9, expectedResult9));

  // Clip with an angle of rect.
  vector<m2::PointD> result10;
  m2::ClipTriangleByRect(r, m2::PointD(-2.0, -0.5), m2::PointD(-0.5, -0.5), m2::PointD(-0.5, -2.0),
                         [&result10](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
  {
    result10.push_back(p1);
    result10.push_back(p2);
    result10.push_back(p3);
  });
  vector<m2::PointD> expectedResult10 = {m2::PointD(-1.0, -0.5), m2::PointD(-0.5, -0.5), m2::PointD(-0.5, -1.0),
                                         m2::PointD(-1.0, -0.5), m2::PointD(-0.5, -1.0), m2::PointD(-1.0, -1.0)};
  TEST(CompareTriangleLists(result10, expectedResult10), (result10, expectedResult10));

  // Clip with 3 angles of rect.
  vector<m2::PointD> result11;
  m2::ClipTriangleByRect(r, m2::PointD(2.0, -3.0), m2::PointD(-2.0, 1.0), m2::PointD(2.0, 2.0),
                         [&result11](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
  {
    result11.push_back(p1);
    result11.push_back(p2);
    result11.push_back(p3);
  });
  vector<m2::PointD> expectedResult11 = {m2::PointD(0.0, -1.0), m2::PointD(-1.0, 0.0), m2::PointD(-1.0, 1.0),
                                         m2::PointD(0.0, -1.0), m2::PointD(-1.0, 1.0), m2::PointD(1.0, 1.0),
                                         m2::PointD(0.0, -1.0), m2::PointD(1.0, 1.0),  m2::PointD(1.0, -1.0)};
  TEST(CompareTriangleLists(result11, expectedResult11), (result11, expectedResult11));
}

UNIT_TEST(Clipping_ClipSplineByRect)
{
  m2::RectD r(-1.0, -1.0, 1.0, 1.0);

  // Intersection.
  m2::SharedSpline spline1;
  spline1.Reset(new m2::Spline(2));
  spline1->AddPoint(m2::PointD(-2.0, 0.0));
  spline1->AddPoint(m2::PointD(2.0, 1.0));
  vector<m2::SharedSpline> result1 = m2::ClipSplineByRect(r, spline1);
  vector<m2::SharedSpline> expectedResult1 = ConstructSplineList({{m2::PointD(-1.0, 0.25), m2::PointD(1.0, 0.75)}});
  TEST(CompareSplineLists(result1, expectedResult1), ());

  // Intersection. Several segments.
  m2::SharedSpline spline2;
  spline2.Reset(new m2::Spline(4));
  spline2->AddPoint(m2::PointD(-2.0, 0.0));
  spline2->AddPoint(m2::PointD(2.0, 1.0));
  spline2->AddPoint(m2::PointD(0.5, -2.0));
  spline2->AddPoint(m2::PointD(-0.5, -0.5));
  vector<m2::SharedSpline> result2 = m2::ClipSplineByRect(r, spline2);
  vector<m2::SharedSpline> expectedResult2 = ConstructSplineList(
      {{m2::PointD(-1.0, 0.25), m2::PointD(1.0, 0.75)}, {m2::PointD(-0.166666666, -1.0), m2::PointD(-0.5, -0.5)}});
  TEST(CompareSplineLists(result2, expectedResult2), ());

  // Completely outside.
  m2::SharedSpline spline3;
  spline3.Reset(new m2::Spline(2));
  spline3->AddPoint(m2::PointD(-2.0, 2.0));
  spline3->AddPoint(m2::PointD(2.0, 3.0));
  vector<m2::SharedSpline> result3 = m2::ClipSplineByRect(r, spline3);
  vector<m2::SharedSpline> expectedResult3 = {};
  TEST(CompareSplineLists(result3, expectedResult3), ());

  // Completely inside.
  m2::SharedSpline spline4;
  spline4.Reset(new m2::Spline(2));
  spline4->AddPoint(m2::PointD(-0.5, 0.0));
  spline4->AddPoint(m2::PointD(0.5, 0.5));
  vector<m2::SharedSpline> result4 = m2::ClipSplineByRect(r, spline4);
  vector<m2::SharedSpline> expectedResult4 = ConstructSplineList({{m2::PointD(-0.5, 0.0), m2::PointD(0.5, 0.5)}});
  TEST(CompareSplineLists(result4, expectedResult4), ());

  // Intersection. Long spline.
  m2::SharedSpline spline5;
  spline5.Reset(new m2::Spline(4));
  spline5->AddPoint(m2::PointD(-2.0, 0.0));
  spline5->AddPoint(m2::PointD(0.0, 0.0));
  spline5->AddPoint(m2::PointD(0.5, 0.5));
  spline5->AddPoint(m2::PointD(2.0, 1.0));
  vector<m2::SharedSpline> result5 = m2::ClipSplineByRect(r, spline5);
  vector<m2::SharedSpline> expectedResult5 = ConstructSplineList(
      {{m2::PointD(-1.0, 0.0), m2::PointD(0.0, 0.0), m2::PointD(0.5, 0.5), m2::PointD(1.0, 0.66666666)}});
  TEST(CompareSplineLists(result5, expectedResult5), ());

  // Intersection. Several segments.
  m2::SharedSpline spline6;
  spline6.Reset(new m2::Spline(5));
  spline6->AddPoint(m2::PointD(-1.0, 0.0));
  spline6->AddPoint(m2::PointD(-0.5, 1.0));
  spline6->AddPoint(m2::PointD(-0.5, 1.000000001));
  spline6->AddPoint(m2::PointD(0.0, 1.5));
  spline6->AddPoint(m2::PointD(0.0, 0.0));
  vector<m2::SharedSpline> result6 = m2::ClipSplineByRect(r, spline6);
  vector<m2::SharedSpline> expectedResult6 = ConstructSplineList(
      {{m2::PointD(-1.0, 0.0), m2::PointD(-0.5, 1.0)}, {m2::PointD(0.0, 1.0), m2::PointD(0.0, 0.0)}});
  TEST(CompareSplineLists(result6, expectedResult6), ());
}
}  // namespace clipping_test
