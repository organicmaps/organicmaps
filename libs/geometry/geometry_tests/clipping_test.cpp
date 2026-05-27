#include "testing/testing.hpp"

#include "geometry/clipping.hpp"

#include <cstddef>
#include <utility>
#include <vector>

namespace clipping_test
{
using namespace std;

double constexpr kClipEps = 1.0E-6;
double constexpr kSplitEps = 1.0E-10;

bool CompareTriangleLists(vector<m2::PointD> const & list1, vector<m2::PointD> const & list2)
{
  if (list1.size() != list2.size())
    return false;

  for (size_t i = 0; i < list1.size(); i++)
    if (!list1[i].EqualDxDy(list2[i], kClipEps))
      return false;

  return true;
}

bool CompareSplineLists(vector<m2::SharedSpline> const & list1, vector<m2::SharedSpline> const & list2)
{
  if (list1.size() != list2.size())
    return false;

  for (size_t i = 0; i < list1.size(); i++)
  {
    auto & path1 = list1[i]->GetPath();
    auto & path2 = list2[i]->GetPath();
    if (path1.size() != path2.size())
      return false;

    for (size_t j = 0; j < path1.size(); j++)
      if (!path1[j].EqualDxDy(path2[j], kClipEps))
        return false;
  }

  return true;
}

m2::SharedSpline MakeSpline(vector<m2::PointD> const & pts)
{
  auto spline = std::make_unique<m2::Spline>(pts.size());
  for (auto const & p : pts)
    spline->AddPoint(p);
  return spline;
}

vector<m2::SharedSpline> ConstructSplineList(vector<vector<m2::PointD>> const & segments)
{
  vector<m2::SharedSpline> result;
  result.reserve(segments.size());
  for (size_t i = 0; i < segments.size(); i++)
    result.push_back(MakeSpline(segments[i]));
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
  m2::SharedSpline spline1 = MakeSpline({{-2.0, 0.0}, {2.0, 1.0}});
  vector<m2::SharedSpline> result1 = m2::ClipSplineByRect(r, spline1);
  vector<m2::SharedSpline> expectedResult1 = ConstructSplineList({{m2::PointD(-1.0, 0.25), m2::PointD(1.0, 0.75)}});
  TEST(CompareSplineLists(result1, expectedResult1), ());

  // Intersection. Several segments.
  m2::SharedSpline spline2 = MakeSpline({{-2.0, 0.0}, {2.0, 1.0}, {0.5, -2.0}, {-0.5, -0.5}});
  vector<m2::SharedSpline> result2 = m2::ClipSplineByRect(r, spline2);
  vector<m2::SharedSpline> expectedResult2 = ConstructSplineList(
      {{m2::PointD(-1.0, 0.25), m2::PointD(1.0, 0.75)}, {m2::PointD(-0.166666666, -1.0), m2::PointD(-0.5, -0.5)}});
  TEST(CompareSplineLists(result2, expectedResult2), ());

  // Completely outside.
  m2::SharedSpline spline3 = MakeSpline({{-2.0, 2.0}, {2.0, 3.0}});
  vector<m2::SharedSpline> result3 = m2::ClipSplineByRect(r, spline3);
  vector<m2::SharedSpline> expectedResult3 = {};
  TEST(CompareSplineLists(result3, expectedResult3), ());

  // Completely inside.
  m2::SharedSpline spline4 = MakeSpline({{-0.5, 0.0}, {0.5, 0.5}});
  vector<m2::SharedSpline> result4 = m2::ClipSplineByRect(r, spline4);
  vector<m2::SharedSpline> expectedResult4 = ConstructSplineList({{m2::PointD(-0.5, 0.0), m2::PointD(0.5, 0.5)}});
  TEST(CompareSplineLists(result4, expectedResult4), ());

  // Intersection. Long spline.
  m2::SharedSpline spline5 = MakeSpline({{-2.0, 0.0}, {0.0, 0.0}, {0.5, 0.5}, {2.0, 1.0}});
  vector<m2::SharedSpline> result5 = m2::ClipSplineByRect(r, spline5);
  vector<m2::SharedSpline> expectedResult5 = ConstructSplineList(
      {{m2::PointD(-1.0, 0.0), m2::PointD(0.0, 0.0), m2::PointD(0.5, 0.5), m2::PointD(1.0, 0.66666666)}});
  TEST(CompareSplineLists(result5, expectedResult5), ());

  // Intersection. Several segments.
  m2::SharedSpline spline6 = MakeSpline({{-1.0, 0.0}, {-0.5, 1.0}, {-0.5, 1.000000001}, {0.0, 1.5}, {0.0, 0.0}});
  vector<m2::SharedSpline> result6 = m2::ClipSplineByRect(r, spline6);
  vector<m2::SharedSpline> expectedResult6 = ConstructSplineList(
      {{m2::PointD(-1.0, 0.0), m2::PointD(-0.5, 1.0)}, {m2::PointD(0.0, 1.0), m2::PointD(0.0, 0.0)}});
  TEST(CompareSplineLists(result6, expectedResult6), ());
}

UNIT_TEST(Clipping_ClipSplineByRect_GrazingRectCorner)
{
  // A two-point spline that just touches a rect corner (and lies entirely
  // outside otherwise) should clip to nothing — the single shared point
  // is degenerate (length 0) and cannot form a valid sub-spline.
  m2::RectD const r(0.0, 0.0, 10.0, 10.0);

  // Touches the top-right corner (10, 10) only.
  m2::SharedSpline const grazeTopRight = MakeSpline({{5.0, 15.0}, {15.0, 5.0}});
  TEST(m2::ClipSplineByRect(r, grazeTopRight).empty(), ());

  // Touches the bottom-left corner (0, 0) only.
  m2::SharedSpline const grazeBottomLeft = MakeSpline({{-1.0, 1.0}, {1.0, -1.0}});
  TEST(m2::ClipSplineByRect(r, grazeBottomLeft).empty(), ());
}

UNIT_TEST(Clipping_ClipSplineByRect_InheritsTangentAndLength)
{
  // Pins the optimization in ClipSplineByRect: an unchanged interior segment
  // (both endpoints inside the rect) must inherit its precomputed direction
  // and length from the source spline byte-for-byte, not via a re-normalize +
  // re-sqrt round trip.
  m2::RectD const r(-1.0, -1.0, 1.0, 1.0);

  // Source: enters the rect from the left, two interior segments, then leaves
  // to the right. The middle two source segments are entirely inside the rect.
  m2::SharedSpline src =
      MakeSpline({m2::PointD(-2.0, 0.0), m2::PointD(-0.5, 0.0), m2::PointD(0.5, 0.5), m2::PointD(2.0, 1.0)});
  auto const result = m2::ClipSplineByRect(r, src);
  TEST_EQUAL(result.size(), 1, ());

  auto const & clipped = *result.front();
  TEST_EQUAL(clipped.GetSize(), 4, ());

  // The middle segment (index 1: (-0.5, 0) -> (0.5, 0.5)) is fully inside
  // the rect and corresponds to source segment index 1. Its direction and
  // length must be exactly the source's — no recomputation.
  auto const srcMiddle = src->GetTangentAndLength(1);
  auto const clippedMiddle = clipped.GetTangentAndLength(1);
  TEST_EQUAL(clippedMiddle.first, srcMiddle.first, ());
  TEST_EQUAL(clippedMiddle.second, srcMiddle.second, ());

  // The first and last segments are clipped: their direction must still match
  // the source (clipping never rotates a segment), but their length is shorter.
  auto const srcFirst = src->GetTangentAndLength(0);
  auto const clippedFirst = clipped.GetTangentAndLength(0);
  TEST_EQUAL(clippedFirst.first, srcFirst.first, ());
  TEST_LESS(clippedFirst.second, srcFirst.second, ());

  auto const srcLast = src->GetTangentAndLength(2);
  auto const clippedLast = clipped.GetTangentAndLength(2);
  TEST_EQUAL(clippedLast.first, srcLast.first, ());
  TEST_LESS(clippedLast.second, srcLast.second, ());
}
UNIT_TEST(ForEachSection_SingleShortSegment)
{
  m2::Spline spl;
  spl.AddPoint(m2::PointD(0, 0));
  spl.AddPoint(m2::PointD(1, 0));

  vector<pair<m2::PointD, m2::PointD>> sections;
  m2::ForEachSection(spl, 5.0, [&](m2::PointD const & p1, m2::PointD const & p2) { sections.emplace_back(p1, p2); });

  TEST_EQUAL(sections.size(), 1, ());
  TEST(sections[0].first.EqualDxDy(m2::PointD(0, 0), kSplitEps), ());
  TEST(sections[0].second.EqualDxDy(m2::PointD(1, 0), kSplitEps), ());
}

UNIT_TEST(ForEachSection_SplitLongSegment)
{
  m2::Spline spl;
  spl.AddPoint(m2::PointD(0, 0));
  spl.AddPoint(m2::PointD(10, 0));

  vector<pair<m2::PointD, m2::PointD>> sections;
  m2::ForEachSection(spl, 3.0, [&](m2::PointD const & p1, m2::PointD const & p2) { sections.emplace_back(p1, p2); });

  // Length 10, maxLength 3 → ceil(10/3) = 4 sections.
  TEST_EQUAL(sections.size(), 4, ());

  // All sections are contiguous.
  for (size_t i = 1; i < sections.size(); ++i)
    TEST(sections[i].first.EqualDxDy(sections[i - 1].second, kSplitEps), ());

  // First starts at origin, last ends at (10, 0).
  TEST(sections.front().first.EqualDxDy(m2::PointD(0, 0), kSplitEps), ());
  TEST(sections.back().second.EqualDxDy(m2::PointD(10, 0), kSplitEps), ());

  // Each section length <= maxLength.
  for (auto const & [p1, p2] : sections)
    TEST_LESS_OR_EQUAL(p1.Length(p2), 3.0 + kSplitEps, ());
}

UNIT_TEST(ForEachSection_MultipleSegments)
{
  // Two segments: short (length 2) + long (length 6), maxLength = 4.
  m2::Spline spl;
  spl.AddPoint(m2::PointD(0, 0));
  spl.AddPoint(m2::PointD(2, 0));
  spl.AddPoint(m2::PointD(2, 6));

  vector<pair<m2::PointD, m2::PointD>> sections;
  m2::ForEachSection(spl, 4.0, [&](m2::PointD const & p1, m2::PointD const & p2) { sections.emplace_back(p1, p2); });

  // First segment (length 2 <= 4): 1 section. Second segment (length 6, ceil(6/4)=2): 2 sections.
  TEST_EQUAL(sections.size(), 3, ());

  TEST(sections[0].first.EqualDxDy(m2::PointD(0, 0), kSplitEps), ());
  TEST(sections[0].second.EqualDxDy(m2::PointD(2, 0), kSplitEps), ());
  TEST(sections[1].first.EqualDxDy(m2::PointD(2, 0), kSplitEps), ());
  TEST(sections[2].second.EqualDxDy(m2::PointD(2, 6), kSplitEps), ());

  for (auto const & [p1, p2] : sections)
    TEST_LESS_OR_EQUAL(p1.Length(p2), 4.0 + kSplitEps, ());
}

UNIT_TEST(ForEachSection_ExactMultiple)
{
  // Segment length exactly divisible by maxLength.
  m2::Spline spl;
  spl.AddPoint(m2::PointD(0, 0));
  spl.AddPoint(m2::PointD(9, 0));

  vector<pair<m2::PointD, m2::PointD>> sections;
  m2::ForEachSection(spl, 3.0, [&](m2::PointD const & p1, m2::PointD const & p2) { sections.emplace_back(p1, p2); });

  TEST_EQUAL(sections.size(), 3, ());
  for (auto const & [p1, p2] : sections)
    TEST_ALMOST_EQUAL_ABS(p1.Length(p2), 3.0, kSplitEps, ());
}

}  // namespace clipping_test
