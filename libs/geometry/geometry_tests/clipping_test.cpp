#include "testing/testing.hpp"

#include "geometry/clipping.hpp"
#include "geometry/robust_orientation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace clipping_test
{
using namespace std;

double constexpr kClipEps = 1.0E-6;
double constexpr kSplitEps = 1.0E-10;

void TestTriangleClip(m2::RectD const & rect, array<m2::PointD, 3> const & input,
                      vector<m2::PointD> const & expectedPolygon)
{
  double expectedArea = 0;
  for (size_t i = 1; i + 1 < expectedPolygon.size(); ++i)
    expectedArea +=
        abs(m2::CrossProduct(expectedPolygon[i] - expectedPolygon[0], expectedPolygon[i + 1] - expectedPolygon[0])) / 2;

  for (bool reverse : {false, true})
    for (size_t shift = 0; shift < input.size(); ++shift)
    {
      auto triangle = input;
      if (reverse)
        swap(triangle[1], triangle[2]);
      rotate(triangle.begin(), triangle.begin() + shift, triangle.end());
      auto const winding = m2::robust::OrientedS(triangle[0], triangle[1], triangle[2]);
      vector<m2::PointD> vertices;
      double area = 0;
      m2::ClipTriangleByRect(rect, triangle[0], triangle[1], triangle[2],
                             [&](auto const & a, auto const & b, auto const & c)
      {
        TEST(rect.IsPointInside(a) && rect.IsPointInside(b) && rect.IsPointInside(c), (a, b, c));
        auto const cross = m2::CrossProduct(b - a, c - a);
        TEST(winding > 0 ? cross >= 0 : cross <= 0, (triangle, a, b, c));
        area += abs(cross) / 2;
        for (auto const & point : {a, b, c})
        {
          TEST(any_of(expectedPolygon.begin(), expectedPolygon.end(),
                      [&](auto const & expected) { return point.EqualDxDy(expected, kClipEps); }),
               (triangle, point, expectedPolygon));
          vertices.push_back(point);
        }
      });
      for (auto const & expected : expectedPolygon)
        TEST(any_of(vertices.begin(), vertices.end(),
                    [&](auto const & point) { return point.EqualDxDy(expected, kClipEps); }),
             (triangle, expected, vertices));
      TEST_ALMOST_EQUAL_ABS(area, expectedArea, 1e-10, (triangle, vertices));
    }
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
  m2::RectD const rect(-1.0, -1.0, 1.0, 1.0);

  // Completely inside.
  TestTriangleClip(rect, {{{0.5, 0.5}, {0.5, -0.5}, {0, 0}}}, {{0.5, 0.5}, {0.5, -0.5}, {0, 0}});
  // One point inside.
  TestTriangleClip(rect, {{{0, 0}, {2, 2}, {2, -2}}}, {{0, 0}, {1, 1}, {1, -1}});
  // Two points inside.
  TestTriangleClip(rect, {{{0, 0.5}, {2, 0}, {0, -0.5}}}, {{0, 0.5}, {1, 0.25}, {1, -0.25}, {0, -0.5}});
  // Two edges clipped.
  TestTriangleClip(rect, {{{0, 0}, {0, 1.5}, {1.5, 0}}}, {{0, 0}, {0, 1}, {0.5, 1}, {1, 0.5}, {1, 0}});
  // Three edges clipped.
  TestTriangleClip(rect, {{{-1.5, 0}, {0, 1.5}, {1.5, 0}}},
                   {{-1, 0.5}, {-0.5, 1}, {0.5, 1}, {1, 0.5}, {1, 0}, {-1, 0}});
  // Seven-vertex intersection exercises the maximum polygon size.
  TestTriangleClip(rect, {{{-2, 0.5}, {0.5, -2}, {2, 2}}},
                   {{-1, -0.5}, {-1, 0.875}, {-2.0 / 3, 1}, {1, 1}, {1, -2.0 / 3}, {0.875, -1}, {-0.5, -1}});
  // Completely outside.
  TestTriangleClip(rect, {{{1.5, 1.5}, {1.5, -1.5}, {2, 0}}}, {});
  // One rectangle corner covered.
  TestTriangleClip(rect, {{{0.5, 0.5}, {0.5, 2}, {2, 0.5}}}, {{0.5, 0.5}, {0.5, 1}, {1, 1}, {1, 0.5}});
  // Rectangle completely covered.
  TestTriangleClip(rect, {{{0, 3}, {5, -2}, {-5, -2}}}, {{-1, 1}, {1, 1}, {1, -1}, {-1, -1}});
  // Bottom-right rectangle corner covered.
  TestTriangleClip(rect, {{{1.5, 0}, {1.5, -1.5}, {0, -1.5}}}, {{0.5, -1}, {1, -0.5}, {1, -1}});
  // Bottom-left rectangle corner covered.
  TestTriangleClip(rect, {{{-2, -0.5}, {-0.5, -0.5}, {-0.5, -2}}}, {{-1, -0.5}, {-0.5, -0.5}, {-0.5, -1}, {-1, -1}});
  // Three rectangle corners covered.
  TestTriangleClip(rect, {{{2, -3}, {-2, 1}, {2, 2}}}, {{0, -1}, {-1, 0}, {-1, 1}, {1, 1}, {1, -1}});
}

UNIT_TEST(Clipping_TriangleTouchesRectangleCorner)
{
  m2::RectD const rect(-1.0, -1.0, 1.0, 1.0);
  TestTriangleClip(rect, {{{-3, -2}, {-1, -1}, {2, 2}}}, {{-1, -0.4}, {0.75, 1}, {1, 1}, {-1, -1}});
}

UNIT_TEST(Clipping_TriangleCornerStaysExact)
{
  m2::RectD const rect(89.296875, 91.40625, 90, 92.109375);
  TestTriangleClip(
      rect,
      {{{89.296875, 91.40625}, {132.81235882391525, 91.792016219990813}, {-177.73787701379288, -166.92971407780541}}},
      {rect.LeftBottom(), {90, 91.412483226649357}, rect.RightBottom()});
}

UNIT_TEST(Clipping_TriangleVerticesOnRectangleEdges)
{
  m2::RectD const rect(-1.0, -1.0, 1.0, 1.0);
  TestTriangleClip(rect, {{{-3, -3}, {-1, 0}, {0, -1}}}, {{-1, -1}, {-1, 0}, {0, -1}});
  TestTriangleClip(rect, {{{-3, -3}, {-1, 0}, {1, 0}}}, {{-1, -1}, {-1, 0}, {1, 0}, {-1.0 / 3, -1}});
  // Touching an edge or corner without covering any area emits no triangles.
  TestTriangleClip(rect, {{{-2, 1}, {2, 1}, {0, 2}}}, {});
  TestTriangleClip(rect, {{{-1, -1}, {-2, -1}, {-1, -2}}}, {});
}

UNIT_TEST(Clipping_DegenerateTriangles)
{
  m2::RectD const rect(-1.0, -1.0, 1.0, 1.0);
  TestTriangleClip(rect, {{{0, 0}, {0, 0}, {0, 0}}}, {});
  TestTriangleClip(rect, {{{-0.5, -0.5}, {-0.5, -0.5}, {0.5, 0.5}}}, {});
  TestTriangleClip(rect, {{{-0.5, -0.5}, {0, 0}, {0.5, 0.5}}}, {});
  TestTriangleClip(rect, {{{-3, -3}, {2, 2}, {-1, -1}}}, {});
  TestTriangleClip(rect, {{{0, -3}, {0, 0}, {0, 3}}}, {});
  TestTriangleClip(rect, {{{2, 2}, {3, 3}, {4, 4}}}, {});
  // A small nonzero area is valid input, independent of the geometry's scale.
  TestTriangleClip(rect, {{{0, 0}, {1, 0}, {0, 1e-12}}}, {{0, 0}, {1, 0}, {0, 1e-12}});
}

UNIT_TEST(Clipping_TriangleWindingPreservesCoveredRectangle)
{
  m2::RectD const rect(3.0, 3.01, 7.0, 7.02);
  std::array<std::array<m2::PointD, 3>, 2> const triangles{
      {{{{0, 0}, {10, 0}, {10, 10.05}}}, {{{0, 0}, {10, 10.05}, {0, 10.05}}}}};
  for (bool clockwise : {false, true})
    for (size_t shift = 0; shift < 3; ++shift)
    {
      double coveredArea = 0;
      for (auto triangle : triangles)
      {
        if (clockwise)
          std::swap(triangle[1], triangle[2]);
        m2::ClipTriangleByRect(rect, triangle[shift], triangle[(shift + 1) % 3], triangle[(shift + 2) % 3],
                               [&](auto const & a, auto const & b, auto const & c)
        {
          TEST(rect.IsPointInside(a) && rect.IsPointInside(b) && rect.IsPointInside(c), (a, b, c));
          auto const cross = m2::CrossProduct(b - a, c - a);
          TEST(clockwise ? cross <= 0 : cross >= 0, (cross));
          coveredArea += std::abs(cross) / 2;
        });
      }
      TEST_ALMOST_EQUAL_ABS(coveredArea, rect.Area(), 1e-10, (clockwise, shift));
    }
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
