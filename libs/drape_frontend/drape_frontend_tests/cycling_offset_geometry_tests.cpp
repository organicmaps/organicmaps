#include "testing/testing.hpp"

#include "drape_frontend/line_shape_helper.hpp"
#include "drape_frontend/stylist.hpp"

#include "geometry/clipping.hpp"
#include "geometry/mercator.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

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

UNIT_TEST(CyclingOffset_LegacyTaperOnlyWithoutApproaches)
{
  m2::SharedSpline source(std::vector<m2::PointD>{{0, 0}, {100, 0}});
  auto taper = [&](std::vector<std::vector<m2::PointD>> const & approaches)
  { return df::MakeJunctionTaper(source, {{50, 0}}, 8, 0.01, approaches); };
  // A legacy payload has junctions but no per-incident geometry.
  TEST_EQUAL(taper({}).m_distances.size(), 1, ());
  TEST_EQUAL(taper({{}}).m_distances.size(), 1, ());
  // When the generator recorded a connector, the connector end moves instead.
  TEST(taper({{{50, 10}}}).m_distances.empty(), ());
  TEST(taper({{{50, 4}}}).m_distances.empty(), ());
  // Every junction is decided independently.
  auto const mixed = df::MakeJunctionTaper(source, {{25, 0}, {75, 0}}, 8, 0.01, {{{25, 5}}, {}});
  TEST_EQUAL(mixed.m_distances.size(), 1, ());
  TEST_ALMOST_EQUAL_ABS(mixed.m_distances[0], 75.0, 1e-9, ());
}

UNIT_TEST(CyclingOffset_ConnectorOnlyGapAndPixelExtrusion)
{
  std::vector<m2::PointD> const road{{-100, 0}, {0, 0}};
  std::vector<m2::PointD> const path{{0, 0}, {10, 10}, {20, 20}};
  std::vector<df::ConnectorTarget> targets{{{0, 0}, road, -8}};
  auto const offsets = df::ConnectorEndpointOffsets(path, targets, 1, 0.001);
  TEST_EQUAL(offsets.size(), path.size(), ());
  TEST_ALMOST_EQUAL_ABS(offsets[0].x, 0.0f, 1e-6f, ());
  TEST_ALMOST_EQUAL_ABS(offsets[0].y, 8.0f, 1e-6f, ());
  TEST_EQUAL(offsets[1], m2::PointF::Zero(), ());
  TEST_EQUAL(offsets[2], m2::PointF::Zero(), ());
  TEST_EQUAL(targets[0].m_roadPath, road, ("Road is immutable"));
  auto const atFractionalZoom = df::ConnectorEndpointOffsets(path, targets, 1.3, 0.001);
  TEST_ALMOST_EQUAL_ABS(atFractionalZoom[0].y, 8.0f, 1e-6f, ("Constant pixels, not map-coordinate displacement"));
  auto const halfway = df::InterpolateLinePixelOffset(path, offsets, 0, {5, 5});
  TEST_ALMOST_EQUAL_ABS(halfway.y, 4.0f, 1e-6f, ("Clipping and arrows use the same interpolated extrusion"));
  TEST(df::ConnectorEndpointOffsets({{0, 0}, {-10, 10}}, targets, 1, 0.001).empty(), ("Already intersects"));
  TEST(df::ConnectorEndpointOffsets({{0, 0}, {10, -10}}, targets, 1, 0.001).empty(), ("No extension across road"));
  TEST(df::ConnectorEndpointOffsets({{0, 0}, {1, 1}}, targets, 1, 0.001).empty(), ("Terminal segment bound"));
  TEST(df::ConnectorEndpointOffsets({{0, 0}, {1, 0}, {10, 10}}, targets, 1, 0.001).empty(),
       ("A later crossing must not add a second kink at a short first segment"));
  TEST(df::ConnectorEndpointOffsets({{0, 0}, {10, 10}, {0, 0}}, targets, 1, 0.001).empty(), ("Closed loop"));
  TEST(df::ConnectorEndpointOffsets({{0, 0}, {10, 10}, {10, 0}, {0, 10}}, targets, 1, 0.001).empty(),
       ("Self-crossing connector"));
  TEST(df::ConnectorEndpointOffsets({{0, 0}, {0, 0}}, targets, 1, 0.001).empty(), ("Zero-length terminal"));
  TEST(df::ConnectorEndpointOffsets(path, std::vector<df::ConnectorTarget>{{{100, 100}, road, -8}}, 1, 0.001).empty(),
       ("Contact at another junction must not move this endpoint"));
  TEST(df::ConnectorEndpointOffsets(path, targets, 0, 0.001).empty(), ());
  TEST(df::ConnectorEndpointOffsets(path, targets, -1, 0.001).empty(), ());
  TEST(df::ConnectorEndpointOffsets(path, {}, 1, 0.001).empty(), ("Legacy geometry stays fixed"));
  std::reverse(targets[0].m_roadPath.begin(), targets[0].m_roadPath.end());
  targets[0].m_offsetPixels = 8;
  auto const reversed = df::ConnectorEndpointOffsets(path, targets, 1, 0.001);
  TEST_EQUAL(reversed, offsets, ());
}

UNIT_TEST(CyclingOffset_ConnectorCausewaySouth)
{
  auto const node = mercator::FromLatLon(49.2944968, -123.1367221);
  auto const next = mercator::FromLatLon(49.2944184, -123.1367418);
  auto const north = mercator::FromLatLon(49.2945315, -123.1367526);
  auto const south = mercator::FromLatLon(49.2944628, -123.1366896);
  std::vector<m2::PointD> const connector{node, next};
  std::vector<df::ConnectorTarget> targets{{node, {north, node}, 10}};
  auto const gap = df::ConnectorEndpointOffsets(connector, targets, 372827, 0.000001);
  TEST_EQUAL(gap.size(), connector.size(), ("Finite incoming road leaves a real gap"));
  TEST_GREATER(gap.front().Length(), 0, ());
  targets.push_back({node, {node, south}, 10});
  TEST(df::ConnectorEndpointOffsets(connector, targets, 372827, 0.000001).empty(),
       ("Outgoing lane already intersects: neither road nor connector moves"));
}

UNIT_TEST(CyclingOffset_ConnectorRejectsNonFiniteParameters)
{
  std::vector<m2::PointD> const path{{0, 0}, {10, 10}};
  std::vector<df::ConnectorTarget> targets{{{0, 0}, {{-100, 0}, {0, 0}}, -8}};
  for (auto const invalid : {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
  {
    TEST(df::ConnectorEndpointOffsets(path, targets, invalid, 0.001).empty(), ());
    TEST(df::ConnectorEndpointOffsets(path, targets, 1, invalid).empty(), ());
  }
  targets[0].m_offsetPixels = std::numeric_limits<float>::infinity();
  TEST(df::ConnectorEndpointOffsets(path, targets, 1, 0.001).empty(), ());
  targets[0].m_offsetPixels = std::numeric_limits<float>::quiet_NaN();
  TEST(df::ConnectorEndpointOffsets(path, targets, 1, 0.001).empty(), ());
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
