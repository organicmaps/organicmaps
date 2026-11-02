#include "testing/testing.hpp"

#include "geometry/clipping.hpp"
#include "geometry/robust_orientation.hpp"
#include "geometry/triangle2d.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <map>
#include <utility>
#include <vector>

namespace clipping_test
{
using namespace std;

double constexpr kClipEps = 1.0E-6;
double constexpr kSplitEps = 1.0E-10;

// Clips |input|, made clockwise, in all cyclic orders of its vertices. The emitted triangles must be inside |rect|,
// clockwise and cover |expectedPolygon| (the convex intersection, in cyclic order) without gaps and overlaps, whatever
// the triangulation is.
void TestTriangleClip(m2::RectD const & rect, array<m2::PointD, 3> input, vector<m2::PointD> const & expectedPolygon)
{
  double expectedOrientation = 0;
  for (size_t i = 2; i < expectedPolygon.size(); ++i)
    expectedOrientation += m2::robust::OrientedS(expectedPolygon[0], expectedPolygon[i - 1], expectedPolygon[i]);
  double const expectedArea = abs(expectedOrientation) / 2;

  if (m2::robust::OrientedS(input[0], input[1], input[2]) > 0)
    swap(input[1], input[2]);

  for (size_t shift = 0; shift < input.size(); ++shift)
  {
    auto triangle = input;
    rotate(triangle.begin(), triangle.begin() + shift, triangle.end());

    // Directed edges between the expected vertices: the internal ones cancel out, the rest must be the boundary.
    map<pair<size_t, size_t>, int> edges;
    auto const addEdge = [&edges](size_t from, size_t to, int count)
    {
      if (from != to)
        edges[minmax(from, to)] += from < to ? count : -count;
    };
    vector<m2::PointD> vertices;
    double area = 0;
    m2::ClipTriangleByRect(rect, triangle[0], triangle[1], triangle[2],
                           [&](m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
    {
      TEST(rect.IsPointInside(a) && rect.IsPointInside(b) && rect.IsPointInside(c), (triangle, a, b, c));
      double const orientation = m2::robust::OrientedS(a, b, c);
      TEST_LESS(orientation, 0, (triangle, a, b, c));
      area -= orientation / 2;

      array<size_t, 3> indices;
      size_t i = 0;
      for (auto const & p : {a, b, c})
      {
        // The closest one tells apart the expected vertices that are closer than kClipEps.
        auto const it = min_element(expectedPolygon.begin(), expectedPolygon.end(), [&p](auto const & l, auto const & r)
        { return p.SquaredLength(l) < p.SquaredLength(r); });
        TEST(it != expectedPolygon.end() && p.EqualDxDy(*it, kClipEps), (triangle, p, expectedPolygon));
        indices[i++] = distance(expectedPolygon.begin(), it);
        vertices.push_back(p);
      }
      for (i = 0; i < indices.size(); ++i)
        addEdge(indices[i], indices[(i + 1) % indices.size()], 1);
    });

    for (auto const & expected : expectedPolygon)
    {
      TEST(any_of(vertices.begin(), vertices.end(),
                  [&expected](auto const & p) { return p.EqualDxDy(expected, kClipEps); }),
           (triangle, expected, vertices));
    }
    int const count = expectedOrientation < 0 ? -1 : 1;
    for (size_t i = 0; i < expectedPolygon.size(); ++i)
      addEdge(i, (i + 1) % expectedPolygon.size(), count);
    for (auto const & [edge, n] : edges)
      TEST_EQUAL(n, 0, (triangle, vertices, edge));

    double const eps = expectedArea == 0 ? 1e-10 : expectedArea * 1e-10;
    TEST_ALMOST_EQUAL_ABS(area, expectedArea, eps, (triangle, vertices));
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
  // 1 point inside.
  TestTriangleClip(rect, {{{0, 0}, {2, 2}, {2, -2}}}, {{0, 0}, {1, 1}, {1, -1}});
  // 2 points inside.
  TestTriangleClip(rect, {{{0, 0.5}, {2, 0}, {0, -0.5}}}, {{0, 0.5}, {1, 0.25}, {1, -0.25}, {0, -0.5}});
  // 2 edges clipping.
  TestTriangleClip(rect, {{{0, 0}, {0, 1.5}, {1.5, 0}}}, {{0, 0}, {0, 1}, {0.5, 1}, {1, 0.5}, {1, 0}});
  // 3 edges clipping.
  TestTriangleClip(rect, {{{-1.5, 0}, {0, 1.5}, {1.5, 0}}},
                   {{-1, 0.5}, {-0.5, 1}, {0.5, 1}, {1, 0.5}, {1, 0}, {-1, 0}});
  // 7 vertices, the maximum.
  TestTriangleClip(rect, {{{-2, 0.5}, {0.5, -2}, {2, 2}}},
                   {{-1, -0.5}, {-1, 0.875}, {-2.0 / 3, 1}, {1, 1}, {1, -2.0 / 3}, {0.875, -1}, {-0.5, -1}});
  // Completely outside.
  TestTriangleClip(rect, {{{1.5, 1.5}, {1.5, -1.5}, {2, 0}}}, {});
  // Covers the top-right corner of rect.
  TestTriangleClip(rect, {{{0.5, 0.5}, {0.5, 2}, {2, 0.5}}}, {{0.5, 0.5}, {0.5, 1}, {1, 1}, {1, 0.5}});
  // Covers rect.
  TestTriangleClip(rect, {{{0, 3}, {5, -2}, {-5, -2}}}, {{-1, 1}, {1, 1}, {1, -1}, {-1, -1}});
  // Covers the bottom-right corner of rect.
  TestTriangleClip(rect, {{{1.5, 0}, {1.5, -1.5}, {0, -1.5}}}, {{0.5, -1}, {1, -0.5}, {1, -1}});
  // Covers the bottom-left corner of rect.
  TestTriangleClip(rect, {{{-2, -0.5}, {-0.5, -0.5}, {-0.5, -2}}}, {{-1, -0.5}, {-0.5, -0.5}, {-0.5, -1}, {-1, -1}});
  // Covers 3 corners of rect.
  TestTriangleClip(rect, {{{2, -3}, {-2, 1}, {2, 2}}}, {{0, -1}, {-1, 0}, {-1, 1}, {1, 1}, {1, -1}});
}

UNIT_TEST(Clipping_ClipTriangleByRect_EdgeThroughCorners)
{
  // The edge goes along the diagonal of rect, from the vertex in its corner.
  TestTriangleClip(m2::RectD(-1.0, -1.0, 1.0, 1.0), {{{-3, -2}, {-1, -1}, {2, 2}}},
                   {{-1, -0.4}, {0.75, 1}, {1, 1}, {-1, -1}});
}

UNIT_TEST(Clipping_ClipTriangleByRect_CornerStaysExact)
{
  m2::RectD const rect(89.296875, 91.40625, 90, 92.109375);
  TestTriangleClip(
      rect,
      {{{89.296875, 91.40625}, {132.81235882391525, 91.792016219990813}, {-177.73787701379288, -166.92971407780541}}},
      {rect.LeftBottom(), {90, 91.412483226649357}, rect.RightBottom()});
}

UNIT_TEST(Clipping_ClipTriangleByRect_ReversedSliver)
{
  // Rounding reverses a fan triangle here (for 2 of the 3 vertex orders), which must not be emitted. The edge from
  // (-1e-14, 135) crosses x = 0 at y = 135 - 8.8e-15, rounded to 135: the polygon gets a zero-width spike to the
  // corner (0, 135), and the fan triangle over it is counter-clockwise.
  TestTriangleClip(m2::RectD(-45, 135, 0, 180), {{{-62.5, 176.25}, {43.5, 96.5}, {-1e-14, 135}}},
                   {{-45, 164.7}, {-45, 176.25 - 79.75 * 17.5 / 106}, {-445.0 / 58, 135}, {-1e-14, 135}});
}

UNIT_TEST(Clipping_ClipTriangleByRect_VerticesOnSides)
{
  m2::RectD const rect(-1.0, -1.0, 1.0, 1.0);
  TestTriangleClip(rect, {{{-3, -3}, {-1, 0}, {0, -1}}}, {{-1, -1}, {-1, 0}, {0, -1}});
  TestTriangleClip(rect, {{{-3, -3}, {-1, 0}, {1, 0}}}, {{-1, -1}, {-1, 0}, {1, 0}, {-1.0 / 3, -1}});

  // Touching a side or a corner without any area emits nothing.
  TestTriangleClip(rect, {{{-2, 1}, {2, 1}, {0, 2}}}, {});
  TestTriangleClip(rect, {{{-1, -1}, {-2, -1}, {-1, -2}}}, {});

  // The vertex on the clipped side is taken as is: its interpolated copy can be out of rect, and the bottom side,
  // which is not clipped, would not cut it.
  TestTriangleClip(m2::RectD(-1, 0, 0, 1), {{{0, 0}, {-1, 1}, {3, 0.1}}}, {{0, 0}, {-1, 1}, {0, 0.775}});
}

UNIT_TEST(Clipping_ClipTriangleByRect_Thin)
{
  // The vertices that are closer than any epsilon are still distinct.
  m2::RectD const rect(-1.0, -1.0, 1.0, 1.0);
  TestTriangleClip(rect, {{{0, 0}, {1, 0}, {0, 1e-12}}}, {{0, 0}, {1, 0}, {0, 1e-12}});
  TestTriangleClip(rect, {{{-2, 0}, {2, 0}, {0, 1e-12}}}, {{-1, 0}, {1, 0}, {1, 5e-13}, {0, 1e-12}, {-1, 5e-13}});
}

UNIT_TEST(Clipping_ClipTriangleByRect_ArcticCoast)
{
  // Decoded WorldCoasts triangles: their common edge is the cell's diagonal, which passes within an ulp of
  // two opposite corners of every tile on it, on any zoom.
  m2::PointD const triangles[][3] = {
      {{-180.0, 157.49999731779064405}, {-180.0, 180.0}, {-157.49999731779067247, 157.49999731779064405}},
      {{-157.49999731779067247, 157.49999731779064405}, {-180.0, 180.0}, {-157.49999731779067247, 180.0}}};

  vector<m2::RectD> tiles;
  for (int zoom = 6; zoom <= 17; ++zoom)
  {
    double const size = 360.0 / (1 << (zoom - 1));
    int const count = static_cast<int>(22.5 / size);
    // Tiles on the diagonal and their neighbours below and to the right.
    for (int i = 0; i < count; ++i)
    {
      for (auto const & [dx, dy] : {pair{0, 0}, pair{0, 1}, pair{1, 0}})
        if (i + dx + dy < count)
          tiles.emplace_back(-180.0 + (i + dx) * size, 180.0 - (i + dy + 1) * size, -180.0 + (i + dx + 1) * size,
                             180.0 - (i + dy) * size);
    }
  }

  for (auto const & tile : tiles)
  {
    for (size_t start = 0; start < 3; ++start)
    {
      double area = 0.0;
      for (auto const & triangle : triangles)
      {
        m2::ClipTriangleByRect(tile, triangle[start], triangle[(start + 1) % 3], triangle[(start + 2) % 3],
                               [&](auto const & a, auto const & b, auto const & c)
        {
          TEST(tile.IsPointInside(a) && tile.IsPointInside(b) && tile.IsPointInside(c), (tile, a, b, c));
          TEST_LESS(m2::robust::OrientedS(a, b, c), 0.0, (tile, a, b, c));
          area += m2::GetTriangleArea(a, b, c);
        });
      }
      TEST_ALMOST_EQUAL_ABS(area, tile.Area(), kSplitEps, (tile, start));
    }
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

UNIT_TEST(Clipping_ClipSplineByRect_SourceOffsets)
{
  m2::RectD const r(-1.0, -1.0, 1.0, 1.0);
  vector<m2::PointD> const points = {{-2.0, 0.0}, {2.0, 1.0}, {0.5, -2.0}, {-0.5, -0.5}};
  auto const spline = MakeSpline(points);

  vector<double> sourceOffsets = {42.0};
  auto const result = m2::ClipSplineByRect(r, spline, sourceOffsets);
  TEST_EQUAL(result.size(), 2, ());
  TEST_EQUAL(sourceOffsets.size(), result.size(), ());

  double const expectedFirstOffset = points[0].Length(result[0]->GetPath().front());
  double const expectedSecondOffset =
      points[0].Length(points[1]) + points[1].Length(points[2]) + points[2].Length(result[1]->GetPath().front());
  TEST_ALMOST_EQUAL_ABS(sourceOffsets[0], expectedFirstOffset, kClipEps, ());
  TEST_ALMOST_EQUAL_ABS(sourceOffsets[1], expectedSecondOffset, kClipEps, ());

  auto const inside = MakeSpline({{-0.5, 0.0}, {0.5, 0.5}});
  sourceOffsets = {42.0};
  TEST_EQUAL(m2::ClipSplineByRect(r, inside, sourceOffsets).size(), 1, ());
  TEST_EQUAL(sourceOffsets, vector<double>{0.0}, ());

  auto const outside = MakeSpline({{-2.0, 2.0}, {2.0, 3.0}});
  sourceOffsets = {42.0};
  TEST(m2::ClipSplineByRect(r, outside, sourceOffsets).empty(), ());
  TEST(sourceOffsets.empty(), ());
}

UNIT_TEST(Clipping_PreSmoothFragments)
{
  m2::RectD const rect(0, 0, 10, 10);
  for (size_t const count : {0, 1, 2, 5})
  {
    vector<m2::PointD> path;
    if (count == 0)
      path = {{20, 20}, {30, 30}};
    for (size_t i = 0; i < count; ++i)
    {
      double const y = i + 1;
      path.insert(path.end(), {{20, y}, {5, y}, {20, y}});
    }

    m2::GuidePointsForSmooth guides;
    m2::PathsForSmooth fragments;
    m2::ClipPathByRectBeforeSmooth(rect, path, guides, fragments);
    TEST_EQUAL(guides.size(), count, ());
    TEST_EQUAL(fragments.size(), count, ());
    for (size_t i = 0; i < count; ++i)
    {
      double const y = i + 1;
      TEST_EQUAL(fragments[i], (vector<m2::PointD>{{20, y}, {5, y}, {20, y}}), (i));
    }

    // Smoothing must preserve every fragment and its endpoints after the inline storage spills.
    m2::SmoothPaths(guides, 4, m2::kCentripetalAlpha, fragments);
    TEST_EQUAL(fragments.size(), count, ());
    for (size_t i = 0; i < count; ++i)
    {
      TEST_GREATER_OR_EQUAL(fragments[i].size(), 3, (i));
      TEST_EQUAL(fragments[i].front(), m2::PointD(20, i + 1), (i));
      TEST_EQUAL(fragments[i].back(), m2::PointD(20, i + 1), (i));
    }
  }
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

  // Source: enters the rect from the left, has one fully interior segment, then leaves to the right.
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
