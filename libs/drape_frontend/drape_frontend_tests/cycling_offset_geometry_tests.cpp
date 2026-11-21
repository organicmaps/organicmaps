#include "testing/testing.hpp"

#include "base/math.hpp"

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

UNIT_TEST(CyclingOffset_AdjacentClipsSharePhaseAtFractionalZoom)
{
  m2::SharedSpline const road(std::vector<m2::PointD>{{0, 0}, {5.5, 0}, {11, 0}});
  std::vector<m2::SplineClipInfo> leftRanges, rightRanges;
  auto const left = m2::ClipSplineByRect({0, -1, 5.5, 1}, road, &leftRanges);
  auto const right = m2::ClipSplineByRect({5.5, -1, 11, 1}, road, &rightRanges);
  TEST_EQUAL(left.size(), 1, ());
  TEST_EQUAL(right.size(), 1, ());
  double constexpr kPeriod = 2;
  for (bool const reversed : {false, true})
  {
    double const leftStart = df::AdvanceDashPhase(31, leftRanges.front().m_startDistance, reversed);
    double const rightStart = df::AdvanceDashPhase(31, rightRanges.front().m_startDistance, reversed);
    double const first = df::AdvanceDashPhase(df::WrapDashPhase(leftStart, kPeriod), left.front()->GetLength(), reversed);
    double const second = df::WrapDashPhase(rightStart, kPeriod);
    TEST_ALMOST_EQUAL_ABS(df::WrapDashPhase(first / kPeriod, 1), second / kPeriod, 1e-9, ());
    // The old shader applied the viewport scale after the two clips wrapped separately.
    TEST(std::abs(df::WrapDashPhase(first / kPeriod * 1.25, 1) -
                  df::WrapDashPhase(second / kPeriod * 1.25, 1)) > 0.1, ());
  }
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
  auto const trim = df::ConnectorEndpointOffsets({{0, 0}, {-10, 10}}, targets, 1, 0.001);
  TEST_EQUAL(trim.size(), 2, ());
  TEST_EQUAL(trim.front(), m2::PointF(-8, 8), ("Trim the nose at the first lane contact"));
  // A contact already on the lane must not be repositioned.
  TEST(df::ConnectorEndpointOffsets({{-8, 8}, {-10, 10}}, targets, 1, 12).empty(), ());
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
  auto const trimmed = df::ConnectorEndpointOffsets(connector, targets, 372827, 0.000001);
  TEST_EQUAL(trimmed.size(), connector.size(), ());
  auto const trim = m2::PointD(trimmed.front().x, trimmed.front().y) / 372827;
  TEST_GREATER(DotProduct(trim, next - node), 0, ("Stop at the existing intersection with the outgoing lane"));
  TEST_ALMOST_EQUAL_ABS(CrossProduct(trim, next - node), 0.0, 1e-12, ());
  TEST_EQUAL(trimmed.back(), m2::PointF::Zero(), ());
  auto const contacts = df::LaneContactDistances({node, south}, {node}, {{next}}, 10, 372827, 0.000001);
  TEST_EQUAL(contacts.size(), 1, ());
  TEST_GREATER(contacts.front(), 0, ());
  TEST_LESS(contacts.front(), (south - node).Length(), ());
}

UNIT_TEST(CyclingOffset_DashContactsUseSameSideAndRenderedGeometry)
{
  std::vector<m2::PointD> road{{0, 0}, {100, 0}};
  auto contacts = [&](std::vector<std::vector<m2::PointD>> const & approaches, double scale = 1)
  { return df::LaneContactDistances(road, {{40, 0}}, approaches, -8, scale, 0.01); };
  TEST_EQUAL(contacts({{{50, 10}}}), std::vector<double>{48}, ());
  TEST_EQUAL(contacts({{{50, 10}}}, 2), std::vector<double>{44}, ());
  TEST(contacts({{{50, 4}}}).empty(), ("Do not paint a dash for an unconfirmed endpoint adjustment"));
  TEST(contacts({{{40, -12}}}).empty(), ("Opposite side"));
  TEST(contacts({{{41, 1}}}).empty(), ("No contact if the endpoint movement is too large"));
  TEST(contacts({}).empty(), ("Legacy data does not anchor dashes"));
  TEST(contacts({{{50, 10}}}, 0).empty(), ());
  auto const reversed =
      df::LaneContactDistances({road.back(), road.front()}, {{40, 0}}, {{{50, 10}}}, 8, 1, 0.01);
  TEST_EQUAL(reversed, std::vector<double>{52}, ());
  TEST_EQUAL(df::LaneContactDistances(road, {{40, 0}}, {{{50, 10}, {50, 10}}}, -8, 1, 0.01).size(), 1, ());
}

UNIT_TEST(CyclingOffset_ConnectorDoesNotGuessBetweenEquallyCloseLanes)
{
  std::vector<m2::PointD> const path{{0, 0}, {10, 10}};
  std::vector<df::ConnectorTarget> targets{{{0, 0}, {{-100, 0}, {0, 0}}, -8},
                                           {{0, 0}, {{0, 0}, {0, -100}}, -8}};
  for (auto const & target : targets)
    TEST(!df::ConnectorEndpointOffsets(path, {target}, 1, 0.01).empty(), ());
  TEST(df::ConnectorEndpointOffsets(path, targets, 1, 0.01).empty(), ());
  std::reverse(targets.begin(), targets.end());
  TEST(df::ConnectorEndpointOffsets(path, targets, 1, 0.01).empty(), ());
}

UNIT_TEST(CyclingOffset_ConnectorPreservesThroughCrossingsAndClearsEveryRoadArm)
{
  df::ConnectorTarget const near{{0, 0}, {{-100, 0}, {0, 0}}, -8};
  df::ConnectorTarget const other{{0, 0}, {{0, 0}, {10, 20}}, -8};
  std::vector<m2::PointD> const merge{{0, 0}, {10, 10}};
  TEST(!df::ConnectorEndpointOffsets(merge, {near}, 1, 0.01).empty(), ());
  TEST(df::ConnectorEndpointOffsets(merge, {near, other}, 1, 0.01).empty(),
       ("The shortest join would cut across the other road arm"));
  TEST(df::ConnectorEndpointOffsets({{0, 0}, {10, 10}, {-5, 4}, {-10, -4}}, {near}, 1, 0.01).empty(),
       ("Keep the geometry of a connector crossing another part of the contacted road"));
}

UNIT_TEST(CyclingOffset_ConnectorCausewayMiddle)
{
  auto const junction = mercator::FromLatLon(49.2964147, -123.1363777);
  auto const ramp = mercator::FromLatLon(49.2963268, -123.1364563);
  std::vector<m2::PointD> const incoming{{mercator::FromLatLon(49.2964552, -123.1363873), junction}};
  std::vector<m2::PointD> const opposite{{ramp, mercator::FromLatLon(49.2962536, -123.1364382)}};
  std::vector<m2::PointD> const path{junction, ramp};
  double constexpr kScale = 372827;
  double constexpr kTolerance = 0.000001;
  auto const joined = df::ConnectorEndpointOffsets(path, {{junction, incoming, 10}}, kScale, kTolerance);
  TEST_EQUAL(joined.size(), path.size(), ("Same-side join at the north end"));
  auto const withRamp =
      df::ConnectorEndpointOffsets(path, {{junction, incoming, 10}, {ramp, opposite, 10}}, kScale, kTolerance);
  TEST_EQUAL(withRamp, joined, ("A lane on the opposite side of the ramp is not an additional join"));
}

UNIT_TEST(CyclingOffset_ConnectorRejectsNonFiniteParameters)
{
  std::vector<m2::PointD> const path{{0, 0}, {10, 10}};
  std::vector<df::ConnectorTarget> targets{{{0, 0}, {{-100, 0}, {0, 0}}, -8}};
  for (auto const invalid : {math::Infinity(), math::Nan()})
  {
    TEST(df::ConnectorEndpointOffsets(path, targets, invalid, 0.001).empty(), ());
    TEST(df::ConnectorEndpointOffsets(path, targets, 1, invalid).empty(), ());
  }
  targets[0].m_offsetPixels = math::Infinity();
  TEST(df::ConnectorEndpointOffsets(path, targets, 1, 0.001).empty(), ());
  targets[0].m_offsetPixels = math::Nan();
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
