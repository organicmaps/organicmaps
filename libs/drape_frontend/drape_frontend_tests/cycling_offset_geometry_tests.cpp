#include "testing/testing.hpp"

#include "drape_frontend/line_shape_helper.hpp"
#include "drape_frontend/stylist.hpp"

#include "geometry/clipping.hpp"

#include <algorithm>
#include <cmath>

UNIT_TEST(CyclingOffset_PixelGapAndDeviceScale)
{
  TEST_EQUAL(df::SideOffsetPixels(10, 2, 2, 1), 8, ());
  TEST_EQUAL(df::SideOffsetPixels(10, 2, 2, -1), -8, ());
  TEST_EQUAL(df::SideOffsetPixels(20, 4, 4, 1), 16, ());
  TEST_EQUAL(df::SideOffsetPixels(10, 2, 2, 0), 0, ());
}

UNIT_TEST(CyclingOffset_ZeroAndBoundedMiter)
{
  std::vector<m2::PointD> path{{0, 0}, {1, 0}, {0, 0.000001}};
  auto const zero = df::LinePixelOffset(path, 1, 0);
  TEST_EQUAL(zero.x, 0, ());
  TEST_EQUAL(zero.y, 0, ());
  auto const corner = df::LinePixelOffset(path, 1, 8);
  TEST(std::isfinite(corner.x) && std::isfinite(corner.y), ());
  TEST_LESS_OR_EQUAL(std::hypot(corner.x, corner.y), 32.001f, ());
}

UNIT_TEST(CyclingOffset_ClipRangesRetainLegacyDistances)
{
  m2::SharedSpline source(std::vector<m2::PointD>{{0, 0}, {10, 0}, {20, 0}});
  std::vector<double> legacy;
  std::vector<m2::SplineClipInfo> ranges;
  auto oldPieces = m2::ClipSplineByRect({5, -1, 15, 1}, source, legacy);
  auto newPieces = m2::ClipSplineByRect({5, -1, 15, 1}, source, &ranges);
  TEST_EQUAL(oldPieces.size(), 1, ());
  TEST_EQUAL(newPieces.size(), oldPieces.size(), ());
  TEST_EQUAL(newPieces[0]->GetPath(), oldPieces[0]->GetPath(), ());
  TEST_ALMOST_EQUAL_ABS(ranges[0].m_startDistance, legacy[0], 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(ranges[0].m_endDistance, 15.0, 1e-9, ());
  TEST_EQUAL(ranges[0].m_startSegment, 0, ());
  TEST_EQUAL(ranges[0].m_endSegment, 1, ());
}

UNIT_TEST(CyclingOffset_ArrowPhasePreservesLeadingOffset)
{
  TEST_ALMOST_EQUAL_ABS(df::PathSymbolOffsetForClip(25, 10, 6), 19.0, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(df::PathSymbolOffsetForClip(3, 10, 6), 7.0, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(df::PathSymbolOffsetForClip(3, 10, 13), 0.0, 1e-9, ());
}

UNIT_TEST(CyclingOffset_TaperOnlyAtAuthorizedPoints)
{
  m2::SharedSpline source(std::vector<m2::PointD>{{0, 0}, {100, 0}});
  auto const taper = df::MakeJunctionTaper(source, {{50, 0}}, 8, 0.01);
  TEST_EQUAL(taper.m_distances.size(), 1, ());
  TEST_ALMOST_EQUAL_ABS(taper.Scale(50), 0.0f, 1e-6f, ());
  TEST_ALMOST_EQUAL_ABS(taper.Scale(46), 0.5f, 1e-6f, ());
  TEST_ALMOST_EQUAL_ABS(taper.Scale(58), 1.0f, 1e-6f, ());
  TEST_ALMOST_EQUAL_ABS(taper.m_spline->GetLength(), source->GetLength(), 1e-9, ());
  auto const unrelated = df::MakeJunctionTaper(source, {{50, 1}}, 8, 0.01);
  TEST(unrelated.m_distances.empty(), ());
  TEST_EQUAL(unrelated.m_spline->GetPath(), source->GetPath(), ());
}

UNIT_TEST(CyclingOffset_TaperRejectsAmbiguousSelfCrossing)
{
  m2::SharedSpline source(std::vector<m2::PointD>{{0, 0}, {10, 10}, {0, 10}, {10, 0}});
  auto const taper = df::MakeJunctionTaper(source, {{5, 5}}, 2, 0.01);
  TEST(taper.m_distances.empty(), ());
}

UNIT_TEST(CyclingOffset_ReversedArrowsFollowTheSameDisplacement)
{
  std::vector<m2::PointD> path{{0, 0}, {10, 0}};
  std::vector<m2::PointF> offsets{{0, 4}, {0, 0}};
  auto forward = df::InterpolateLinePixelOffset(path, offsets, 0, {2, 0});
  std::reverse(path.begin(), path.end());
  std::reverse(offsets.begin(), offsets.end());
  auto reverse = df::InterpolateLinePixelOffset(path, offsets, 0, {2, 0});
  TEST_ALMOST_EQUAL_ABS(forward.y, 3.2f, 1e-6f, ());
  TEST_ALMOST_EQUAL_ABS(forward.y, reverse.y, 1e-6f, ());
  df::Stylist::SideRules rules;
  rules.m_reverseArrow = true;
  auto arrow = rules.GetArrowSpline(m2::SharedSpline(path));
  TEST_EQUAL(arrow->GetPath().front(), path.back(), ());
}
