#include "testing/testing.hpp"

#include "drape_frontend/apply_feature_params.hpp"
#include "drape_frontend/clip_splines_builder.hpp"
#include "drape_frontend/line_shape_helper.hpp"
#include "drape_frontend/stylist.hpp"
#include "drape_frontend/tile_key.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/cycling_infrastructure.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/map_style_reader.hpp"

#include "geometry/clipping.hpp"

#include <cmath>
#include <set>
#include <vector>

// Friend of FeatureType (declared in indexer/feature.hpp). Lets a test set up
// a fully-formed line feature without going through a real .mwm load.
/// @todo Factor out as a reusable class into some xxx_test_support.
class MockTestFeature : public FeatureType
{
public:
  explicit MockTestFeature(std::vector<m2::PointD> const & points, uint32_t type = 0) : FeatureType(FeatureID{}, type)
  {
    m_header = static_cast<uint8_t>(feature::HeaderGeomType::Line);
    m_points.assign(points.begin(), points.end());
    m_limitRect.MakeEmpty();
    feature::CalcRect(m_points, m_limitRect);
    m_parsed.m_points = true;
    m_parsed.m_triangles = true;
    m_parsed.m_metadata = true;
    m_parsed.m_metaIds = true;
    m_parsed.m_relations = true;
  }

  void AddStyleType(uint32_t type)
  {
    auto const count = GetTypesCount();
    CHECK_LESS(count, m_types.size(), ());
    m_types[count] = type;
    m_header = (m_header & ~feature::HEADER_MASK_TYPE) | count;
  }

  void SetCyclingSides(cycling::Kind left, cycling::Kind right,
                       cycling::Direction leftDirection = cycling::Direction::Unknown,
                       cycling::Direction rightDirection = cycling::Direction::Unknown)
  {
    m_cyclingInfrastructure = cycling::Encode({{left, leftDirection}, {right, rightDirection}});
  }
};

namespace clip_spline_builder_tests
{
using P = m2::PointD;

// Builds an ApplyFeatureParams configured for the given zoom level, with a
// tile rect large enough to contain all of the test points (so the limit
// rect short-circuit doesn't drop the feature in the algorithm tests). The
// minSegmentSqr is overridden directly so the tests don't depend on the
// exact derived value from VisualParams.
df::ApplyFeatureParams MakeParams(uint8_t zoomLevel, double minSegmentSqr)
{
  df::ApplyFeatureParams params;
  params.m_tileKey = df::TileKey(0, 0, zoomLevel);
  // Tile rect that comfortably contains the test points (which all live in [-100, 100]).
  params.m_tileRect = m2::RectD(-1000.0, -1000.0, 1000.0, 1000.0);
  params.m_minSegmentSqrLength = minSegmentSqr;
  return params;
}

// Convenience: simplification kicks in for zoom levels 10..12.
constexpr uint8_t kSimplifyZoom = 11;
// And does NOT for other zoom levels.
constexpr uint8_t kNoSimplifyZoom = 14;

// Threshold used by most algorithm tests: minSqr = 1.0, so anything closer
// than 1 unit (squared distance) gets thinned.
constexpr double kMinSqr = 1.0;

void TestPath(df::ClipSplinesBuilder const & builder, std::vector<P> const & expected)
{
  auto const & got = builder.GetPath();
  TEST_EQUAL(got.size(), expected.size(), (got, expected));
  for (size_t i = 0; i < expected.size(); ++i)
    TEST_EQUAL(got[i], expected[i], (i, got, expected));
}

UNIT_TEST(ClipSplinesBuilder_CyclingColdStart)
{
  auto & reader = GetStyleReader();
  auto const previous = reader.GetCurrentStyle();
  reader.SetCurrentStyle(MapStyleCyclingDark);
  classificator::Load();
  TEST(classificator::IsStyleLoaded(MapStyleDefaultLight), ());
  TEST(classificator::IsStyleLoaded(MapStyleDefaultDark), ());
  auto const road = classif().GetTypeByPath({"highway", "secondary"});
  MockTestFeature feature({P(0, 0), P(10, 0)}, road);
  feature.SetCyclingSides(cycling::Kind::Lane, cycling::Kind::Shared);
  df::Stylist result(feature, 18, 0, MapStyleCyclingDark);
  TEST_EQUAL(result.m_cyclewaySides.size(), 2, ());
  TEST(!result.m_lineRules.empty(), ());
  reader.SetCurrentStyle(previous);
  classificator::Load();
}

UNIT_TEST(ClipSplinesBuilder_CyclingCoreWidths)
{
  classificator::Load();
  classificator::EnsureStyleLoaded(MapStyleDefaultLight);
  auto const & cl = classif(MapStyleDefaultLight);
  for (auto const type :
       {cl.GetTypeByPath({"highway", "cycleway"}), cl.GetTypeByPath({"highway", "footway", "bicycle"})})
  {
    for (auto const & styles :
         {std::pair{MapStyleDefaultLight, MapStyleCyclingLight}, std::pair{MapStyleDefaultDark, MapStyleCyclingDark}})
    {
      for (uint8_t const zoom : {14, 16, 18})
      {
        MockTestFeature feature({P(0, 0), P(10, 0)}, type);
        df::Stylist standard(feature, zoom, 0, styles.first);
        df::Stylist cycling(feature, zoom, 0, styles.second);
        std::set<drule::LineRule const *> standardLines, cyclingLines;
        for (auto const rule : standard.m_lineRules)
          if (!rule->pathsym)
            standardLines.insert(rule);
        for (auto const rule : cycling.m_lineRules)
          if (!rule->pathsym)
            cyclingLines.insert(rule);
        TEST(!cyclingLines.empty(), (type, zoom));
        for (auto const rule : standardLines)
          TEST_EQUAL(rule->width, df::Stylist::CyclewayIndicatorWidth(zoom), ());
        for (auto const rule : cyclingLines)
          TEST_EQUAL(rule->width, df::Stylist::CyclewayIndicatorWidth(zoom), ());
      }
    }
  }
}

UNIT_TEST(ClipSplinesBuilder_CyclingZoomTransition)
{
  classificator::Load();
  classificator::EnsureStyleLoaded(MapStyleDefaultLight);
  auto const & cl = classif(MapStyleDefaultLight);
  auto const road = cl.GetTypeByPath({"highway", "secondary"});
  using Kind = cycling::Kind;
  struct Case
  {
    Kind left, right, summary;
    size_t sides;
  };
  auto const lineSet = [](df::Stylist const & stylist)
  { return std::set<drule::LineRule const *>(stylist.m_lineRules.begin(), stylist.m_lineRules.end()); };
  for (auto const & item :
       {Case{Kind::Lane, Kind::Shared, Kind::Shared, 2}, Case{Kind::Shared, Kind::Shared, Kind::Shared, 1},
        Case{Kind::Shared, Kind::Track, Kind::Shared, 2}, Case{Kind::Track, Kind::Lane, Kind::Lane, 2},
        Case{Kind::None, Kind::Track, Kind::Track, 1}})
  {
    auto const summary = cl.GetTypeByPath({"cyclewaytag", cycling::TypeName(item.summary)});
    for (auto const style : {MapStyleDefaultLight, MapStyleCyclingLight, MapStyleDefaultDark, MapStyleCyclingDark})
    {
      for (uint8_t const zoom : {13, 14, 16, 17, 18})
      {
        MockTestFeature feature({P(0, 0), P(10, 0)}, road);
        feature.AddStyleType(summary);
        feature.SetCyclingSides(item.left, item.right);
        df::Stylist result(feature, zoom, 0, style);
        if (zoom < df::Stylist::kCyclewaySideMinZoom)
        {
          MockTestFeature expected({P(0, 0), P(10, 0)}, road);
          expected.AddStyleType(summary);
          df::Stylist summaryStyle(expected, zoom, 0, style);
          TEST(result.m_cyclewaySides.empty(), (zoom, style));
          if (item.summary == Kind::Track)
          {
            for (auto const rule : result.m_lineRules)
              if (!rule->pathsym)
                TEST_EQUAL(rule->width, df::Stylist::CyclewayIndicatorWidth(zoom), ());
          }
          else
          {
            MockTestFeature plainRoad({P(0, 0), P(10, 0)}, road);
            df::Stylist plainStyle(plainRoad, zoom, 0, style);
            TEST(lineSet(result) == lineSet(plainStyle), (zoom, style));
          }
        }
        else
        {
          MockTestFeature expected({P(0, 0), P(10, 0)}, road);
          auto const normalStyle = MapStyleIsDark(style) ? MapStyleDefaultDark : MapStyleDefaultLight;
          df::Stylist roadStyle(expected, zoom, 0, normalStyle);
          TEST_EQUAL(result.m_cyclewaySides.size(), item.sides, (zoom, style));
          TEST(lineSet(result) == lineSet(roadStyle), (zoom, style));
          if (item.sides == 0)
            continue;
          for (auto const & side : result.m_cyclewaySides)
          {
            TEST_EQUAL(side.m_rules.size(), 1, ());
            TEST_EQUAL(side.m_rules.front()->width, df::Stylist::CyclewayIndicatorWidth(zoom), ());
            auto const kind = side.m_side == 0 ? Kind::Shared : (side.m_side < 0 ? item.left : item.right);
            auto const rule = side.m_rules.front();
            TEST_EQUAL(rule->dashdot.has_value(), kind != Kind::Track, ());
            if (rule->dashdot)
            {
              TEST_EQUAL(rule->dashdot->dd.size(), 2, ());
              if (kind == Kind::Shared)
                TEST_LESS(rule->dashdot->dd.front(), rule->width * 1.1, ());
              else
                TEST_GREATER(rule->dashdot->dd.front(), rule->width * 3, ());
            }
          }
          std::vector<int> expectedSides;
          if (item.left != Kind::None && item.left != Kind::Shared)
            expectedSides.push_back(-1);
          if (item.right != Kind::None && item.right != Kind::Shared)
            expectedSides.push_back(1);
          if (item.left == Kind::Shared || item.right == Kind::Shared)
            expectedSides.push_back(0);
          for (size_t i = 0; i < expectedSides.size(); ++i)
            TEST_EQUAL(result.m_cyclewaySides[i].m_side, expectedSides[i], ());
        }
      }
    }
  }
}

UNIT_TEST(ClipSplinesBuilder_CyclingZoomSummaryWithoutAggregateType)
{
  classificator::Load();
  classificator::EnsureStyleLoaded(MapStyleDefaultLight);
  auto const & cl = classif(MapStyleDefaultLight);
  auto const road = cl.GetTypeByPath({"highway", "secondary"});
  auto const lane = cl.GetTypeByPath({"cyclewaytag", "lane"});
  for (auto const style : {MapStyleDefaultLight, MapStyleCyclingLight, MapStyleDefaultDark, MapStyleCyclingDark})
  {
    MockTestFeature feature({P(0, 0), P(10, 0)}, road);
    feature.SetCyclingSides(cycling::Kind::Track, cycling::Kind::Lane);
    MockTestFeature expected({P(0, 0), P(10, 0)}, road);
    expected.AddStyleType(lane);
    df::Stylist result(feature, 16, 0, style);
    df::Stylist summaryStyle(expected, 16, 0, style);
    TEST_EQUAL(result.m_cyclewaySides.size(), 2, (style));
    for (auto const & side : result.m_cyclewaySides)
      TEST_EQUAL(side.m_rules.front()->width, df::Stylist::CyclewayIndicatorWidth(16), ());
  }
}

UNIT_TEST(ClipSplinesBuilder_CyclingDirectionSymbols)
{
  classificator::Load();
  classificator::EnsureStyleLoaded(MapStyleDefaultLight);
  auto const & cl = classif(MapStyleDefaultLight);
  auto const symbols = [](df::Stylist const & stylist)
  {
    std::multiset<std::string> result;
    for (auto const rule : stylist.m_lineRules)
      if (rule->pathsym && rule->pathsym->name.find("cycling-arrow-") == 0)
        result.insert(rule->pathsym->name);
    return result;
  };
  for (auto const facility :
       {cl.GetTypeByPath({"highway", "cycleway"}), cl.GetTypeByPath({"cyclewaytag", "track"}),
        cl.GetTypeByPath({"cyclewaytag", "lane"}), cl.GetTypeByPath({"cyclewaytag", "shared_lane"})})
  {
    for (auto const direction : {"oneway", "onedir_bicycle", "bidir_bicycle"})
    {
      for (uint8_t const zoom : {14, 15, 16, 17, 18})
      {
        MockTestFeature feature({P(0, 0), P(10, 0)}, cl.GetTypeByPath({"highway", "secondary"}));
        feature.AddStyleType(facility);
        feature.AddStyleType(cl.GetTypeByPath({"hwtag", direction}));
        if (std::string(direction) == "bidir_bicycle")
          feature.AddStyleType(cl.GetTypeByPath({"hwtag", "oneway"}));
        for (auto const & styles : {std::pair{MapStyleDefaultLight, MapStyleCyclingLight},
                                    std::pair{MapStyleDefaultDark, MapStyleCyclingDark}})
        {
          df::Stylist reference(feature, zoom, 0, styles.second);
          auto const wanted = symbols(reference);
          TEST_EQUAL(wanted.empty(), zoom < 15 || std::string(direction) == "bidir_bicycle", (zoom, direction));
          df::Stylist standard(feature, zoom, 0, styles.first);
          df::Stylist cycling(feature, zoom, 0, styles.second);
          TEST(symbols(standard) == wanted, (zoom, direction, facility));
          TEST(symbols(cycling) == wanted, (zoom, direction, facility));
        }
      }
    }
  }
}

UNIT_TEST(ClipSplinesBuilder_CyclingSideDirectionSymbols)
{
  classificator::Load();
  using Kind = cycling::Kind;
  using Direction = cycling::Direction;
  auto const & cl = classif();
  for (auto const kind : {Kind::Track, Kind::Lane})
    for (auto const direction : {Direction::Unknown, Direction::Forward, Direction::Backward, Direction::Both})
      for (int flags = 0; flags != 4; ++flags)
        for (uint8_t const zoom : {14, 15, 18})
          for (auto const style :
               {MapStyleDefaultLight, MapStyleDefaultDark, MapStyleCyclingLight, MapStyleCyclingDark})
          {
            MockTestFeature feature({P(0, 0), P(10, 0)}, cl.GetTypeByPath({"highway", "secondary"}));
            feature.AddStyleType(cl.GetTypeByPath({"cyclewaytag", cycling::TypeName(kind)}));
            if (flags == 1 || flags == 2)
              feature.AddStyleType(cl.GetTypeByPath({"hwtag", "oneway"}));
            if (flags == 2)
              feature.AddStyleType(cl.GetTypeByPath({"hwtag", "bidir_bicycle"}));
            if (flags == 3)
              feature.AddStyleType(cl.GetTypeByPath({"hwtag", "onedir_bicycle"}));
            feature.SetCyclingSides(kind, kind, direction, Direction::Both);
            df::Stylist result(feature, zoom, 0, style);
            TEST_EQUAL(result.m_cyclewaySides.size(), 2, ());
            auto const & left = result.m_cyclewaySides[0];
            auto const & right = result.m_cyclewaySides[1];
            bool const expected = zoom >= 15 && (direction == Direction::Forward || direction == Direction::Backward ||
                                                 (direction == Direction::Unknown && (flags == 1 || flags == 3)));
            TEST_EQUAL(left.m_arrowRule != nullptr, expected,
                       (static_cast<char>(kind), static_cast<char>(direction), flags, zoom, style));
            TEST(right.m_arrowRule == nullptr, ());
            TEST_EQUAL(left.m_side, -1, ());
            TEST_EQUAL(right.m_side, 1, ());
            if (expected)
            {
              TEST_EQUAL(left.m_reverseArrow, direction == Direction::Backward, ());
              auto const prefix = kind == Kind::Track ? "cycling-arrow-solid-" : "cycling-arrow-patterned-";
              TEST(left.m_arrowRule->pathsym->name.starts_with(prefix), ());
            }
            for (auto const rule : result.m_lineRules)
              TEST(!rule->pathsym || !rule->pathsym->name.starts_with("cycling-arrow-"), ());
          }
}

UNIT_TEST(ClipSplinesBuilder_SharedCenterlineDirections)
{
  classificator::Load();
  using Kind = cycling::Kind;
  using Direction = cycling::Direction;
  auto const & cl = classif();
  for (auto const left : {Direction::Unknown, Direction::Forward, Direction::Backward, Direction::Both})
    for (auto const right : {Direction::Unknown, Direction::Forward, Direction::Backward, Direction::Both})
      for (bool const roadOneWay : {false, true})
        for (uint8_t const zoom : {14, 18})
          for (auto const style :
               {MapStyleDefaultLight, MapStyleDefaultDark, MapStyleCyclingLight, MapStyleCyclingDark})
          {
            MockTestFeature feature({P(0, 0), P(10, 0)}, cl.GetTypeByPath({"highway", "secondary"}));
            feature.AddStyleType(cl.GetTypeByPath({"cyclewaytag", "shared_lane"}));
            if (roadOneWay)
              feature.AddStyleType(cl.GetTypeByPath({"hwtag", "onedir_bicycle"}));
            feature.SetCyclingSides(Kind::Shared, Kind::Shared, left, right);
            df::Stylist result(feature, zoom, 0, style);
            TEST_EQUAL(result.m_cyclewaySides.size(), 1, ());
            auto const & center = result.m_cyclewaySides.front();
            TEST_EQUAL(center.m_side, 0, ());
            TEST_EQUAL(center.m_rules.size(), 1, ());
            TEST_EQUAL(center.m_rules.front()->width, df::Stylist::CyclewayIndicatorWidth(zoom), ());
            auto const resolve = [roadOneWay](Direction direction)
            { return direction == Direction::Unknown && roadOneWay ? Direction::Forward : direction; };
            auto const direction = resolve(left);
            bool const expected = zoom >= 15 && direction == resolve(right) &&
                                  (direction == Direction::Forward || direction == Direction::Backward);
            TEST_EQUAL(center.m_arrowRule != nullptr, expected,
                       (static_cast<char>(left), static_cast<char>(right), roadOneWay, zoom, style));
            if (expected)
              TEST_EQUAL(center.m_reverseArrow, direction == Direction::Backward, ());
            for (auto const rule : result.m_lineRules)
              TEST(!rule->pathsym || !rule->pathsym->name.starts_with("cycling-arrow-"), ());
          }
}

UNIT_TEST(ClipSplinesBuilder_MixedSharedCenterlineArrows)
{
  classificator::Load();
  using Kind = cycling::Kind;
  using Direction = cycling::Direction;
  auto const & cl = classif();
  for (auto const separate : {Kind::Track, Kind::Lane})
    for (bool const sharedOnLeft : {false, true})
      for (auto const style : {MapStyleDefaultLight, MapStyleDefaultDark, MapStyleCyclingLight, MapStyleCyclingDark})
      {
        MockTestFeature feature({P(0, 0), P(10, 0)}, cl.GetTypeByPath({"highway", "secondary"}));
        feature.AddStyleType(cl.GetTypeByPath({"cyclewaytag", "shared_lane"}));
        feature.SetCyclingSides(sharedOnLeft ? Kind::Shared : separate, sharedOnLeft ? separate : Kind::Shared,
                                sharedOnLeft ? Direction::Backward : Direction::Forward,
                                sharedOnLeft ? Direction::Forward : Direction::Backward);
        df::Stylist result(feature, 18, 0, style);
        TEST_EQUAL(result.m_cyclewaySides.size(), 2, ());
        auto const & offset = result.m_cyclewaySides[0];
        auto const & center = result.m_cyclewaySides[1];
        TEST_EQUAL(offset.m_side, sharedOnLeft ? 1 : -1, ());
        TEST_EQUAL(center.m_side, 0, ());
        TEST(offset.m_arrowRule && center.m_arrowRule, ());
        TEST(!offset.m_reverseArrow && center.m_reverseArrow, ());
        TEST_EQUAL(offset.m_rules.front()->width, center.m_rules.front()->width, ());
        TEST(center.m_rules.front()->dashdot.has_value(), ());
      }
}

UNIT_TEST(ClipSplinesBuilder_CyclingSideArrowGeometry)
{
  m2::SharedSpline const piece(std::vector<P>{P(0, 2), P(5, 2), P(10, 2)});
  df::Stylist::SideRules side{-1, {}};
  auto const forward = side.GetArrowSpline(piece);
  TEST(forward->GetPath() == piece->GetPath(), ());
  side.m_reverseArrow = true;
  auto const backward = side.GetArrowSpline(piece);
  TEST_EQUAL(backward->GetPath().front(), P(10, 2), ());
  TEST_EQUAL(backward->GetPath().back(), P(0, 2), ());
  TEST_EQUAL(piece->GetPath().front(), P(0, 2), ());
  TEST_EQUAL(piece->GetPath().back(), P(10, 2), ());
}

UNIT_TEST(ClipSplinesBuilder_CyclingBridgeContinuity)
{
  classificator::Load();
  auto const & cl = classif();
  bool sawDecoration = false;
  for (auto const name : {"motorway", "primary", "secondary", "residential", "service", "footway"})
    for (uint8_t const zoom : {14, 16, 17, 18, 19, 20})
      for (auto const kind : {cycling::Kind::Track, cycling::Kind::Lane, cycling::Kind::Shared})
        for (auto const style : {MapStyleDefaultLight, MapStyleDefaultDark, MapStyleCyclingLight, MapStyleCyclingDark})
        {
          auto const roadType = cl.GetTypeByPath({"highway", name});
          auto const bridgeType = cl.GetTypeByPath({"highway", name, "bridge"});
          MockTestFeature plain({P(0, 0), P(10, 0)}, roadType);
          MockTestFeature bridge({P(0, 0), P(10, 0)}, bridgeType);
          for (auto * feature : {&plain, &bridge})
          {
            feature->AddStyleType(cl.GetTypeByPath({"cyclewaytag", cycling::TypeName(kind)}));
            feature->SetCyclingSides(kind, cycling::Kind::None, cycling::Direction::Forward);
          }
          df::Stylist before(plain, zoom, 0, style);
          df::Stylist onBridge(bridge, zoom, 0, style);
          TEST_EQUAL(before.m_cyclewaySides.size(), 1, (name, zoom, style));
          TEST_EQUAL(onBridge.m_cyclewaySides.size(), 1, (name, zoom, style));
          auto const & a = before.m_cyclewaySides.front();
          auto const & b = onBridge.m_cyclewaySides.front();
          float ordinaryWidth = 0;
          for (auto const rule : before.m_lineRules)
            if (!rule->pathsym)
              ordinaryWidth = std::max(ordinaryWidth, rule->width);
          TEST_EQUAL(a.m_roadWidth, ordinaryWidth, (name, zoom));
          TEST_EQUAL(a.m_roadWidth, b.m_roadWidth, (name, zoom, style));
          TEST_EQUAL(a.m_side, b.m_side, ());
          TEST_EQUAL(a.m_rules.front()->width, b.m_rules.front()->width, ());
          TEST_EQUAL(a.m_arrowRule, b.m_arrowRule, ());
          for (float const density : {1.0f, 3.0f})
            TEST_EQUAL(df::SideOffsetPixels(a.m_roadWidth * density, a.m_rules.front()->width * density, 2.0f * density,
                                            a.m_side),
                       df::SideOffsetPixels(b.m_roadWidth * density, b.m_rules.front()->width * density, 2.0f * density,
                                            b.m_side),
                       ());

          // Do not achieve continuity by hiding or replacing bridge drawing rules.
          MockTestFeature bridgeOnly({P(0, 0), P(10, 0)}, bridgeType);
          auto const standardStyle = MapStyleIsDark(style) ? MapStyleDefaultDark : MapStyleDefaultLight;
          df::Stylist expected(bridgeOnly, zoom, 0, standardStyle);
          std::set<drule::LineRule const *> actual(onBridge.m_lineRules.begin(), onBridge.m_lineRules.end());
          std::set<drule::LineRule const *> original(expected.m_lineRules.begin(), expected.m_lineRules.end());
          TEST(actual == original, (name, zoom, style));
          for (auto const rule : onBridge.m_lineRules)
          {
            if (rule->pathsym || rule->width <= b.m_roadWidth)
              continue;
            sawDecoration = true;
            TEST_GREATER(b.m_rules.front()->priority, rule->priority, (name, zoom, style));
          }
        }
  TEST(sawDecoration, ());
}

UNIT_TEST(ClipSplinesBuilder_CyclingTunnelContinuity)
{
  classificator::Load();
  auto const & cl = classif();
  for (auto const name : {"motorway", "primary", "secondary", "residential"})
    for (uint8_t const zoom : {14, 17, 19})
      for (auto const style : {MapStyleDefaultLight, MapStyleDefaultDark, MapStyleCyclingLight, MapStyleCyclingDark})
      {
        MockTestFeature plain({P(0, 0), P(10, 0)}, cl.GetTypeByPath({"highway", name}));
        MockTestFeature tunnel({P(0, 0), P(10, 0)}, cl.GetTypeByPath({"highway", name, "tunnel"}));
        for (auto * feature : {&plain, &tunnel})
          feature->SetCyclingSides(cycling::Kind::Lane, cycling::Kind::None);
        df::Stylist a(plain, zoom, 0, style), b(tunnel, zoom, 0, style);
        TEST_EQUAL(a.m_cyclewaySides.size(), 1, ());
        TEST_EQUAL(b.m_cyclewaySides.size(), 1, ());
        TEST_EQUAL(a.m_cyclewaySides.front().m_roadWidth, b.m_cyclewaySides.front().m_roadWidth, (name, zoom, style));
      }
}

UNIT_TEST(ClipSplinesBuilder_OnlyCyclingIndicator)
{
  classificator::Load();
  auto const road = classif().GetTypeByPath({"highway", "service"});
  for (auto const style : {MapStyleDefaultLight, MapStyleDefaultDark, MapStyleCyclingLight, MapStyleCyclingDark})
  {
    MockTestFeature feature({P(0, 0), P(10, 0)}, road);
    feature.SetCyclingSides(cycling::Kind::Lane, cycling::Kind::None);
    df::Stylist result(feature, 14, 0, style);
    TEST(result.m_lineRules.empty(), ());
    TEST(result.HasLineRules(), ());
    TEST_EQUAL(result.m_cyclewaySides.size(), 1, ());
    TEST(!result.m_cyclewaySides.front().m_rules.empty(), ());
  }
}

UNIT_TEST(ClipSplinesBuilder_RepeatedSourceOffsets)
{
  auto params = MakeParams(kNoSimplifyZoom, 0);
  params.m_tileRect = m2::RectD(-1, -1, 1, 1);
  MockTestFeature feature({P(-2, 0), P(2, 0), P(-2, 0), P(2, 0)});
  df::ClipSplinesBuilder builder(params);
  builder.Build(feature, kNoSimplifyZoom);
  auto const pieces = builder.Release(false, true);
  TEST_EQUAL(pieces.size(), 3, ());
  for (size_t i = 0; i < pieces.size(); ++i)
    TEST_ALMOST_EQUAL_ABS(builder.GetSplineOffsets()[i], 1.0 + 4.0 * i, 1e-9, ());
}

UNIT_TEST(ClipSplinesBuilder_ArrowPhaseAcrossCuts)
{
  for (bool const reverse : {false, true})
    for (auto const range : {std::pair{0.0, 100.0}, std::pair{100.0, 200.0}, std::pair{200.0, 300.0}})
    {
      double const start = reverse ? 300.0 - range.second : range.first;
      auto const first = df::PathSymbolOffsetForClip(90.0, 74.0, start);
      for (double local = first; local <= range.second - range.first; local += 74.0)
      {
        double const oriented = start + local;
        TEST_GREATER_OR_EQUAL(oriented, 90.0, ());
        TEST_ALMOST_EQUAL_ABS(std::fmod(oriented - 90.0, 74.0), 0.0, 1e-9, ());
      }
    }
  TEST_EQUAL(df::PathSymbolOffsetForClip(90, 74, 0), 90.0, ());
  TEST_EQUAL(df::PathSymbolOffsetForClip(90, 74, 100), 64.0, ());
}

UNIT_TEST(ClipSplinesBuilder_CornerContextAcrossCuts)
{
  m2::SharedSpline source(std::vector<P>{P(0, 0), P(10, 0), P(10, 10)});
  std::vector<m2::SplineClipInfo> a, b;
  auto const left = m2::ClipSplineByRect(m2::RectD(0, -1, 10, 0), source, &a);
  auto const right = m2::ClipSplineByRect(m2::RectD(10, 0, 11, 10), source, &b);
  TEST_EQUAL(left.size(), 1, ());
  TEST_EQUAL(right.size(), 1, ());
  auto const end = df::LinePixelOffsetOnSegment(source->GetPath(), a[0].m_endSegment, left[0]->GetPath().back(), 5);
  auto const start =
      df::LinePixelOffsetOnSegment(source->GetPath(), b[0].m_startSegment, right[0]->GetPath().front(), 5);
  TEST_ALMOST_EQUAL_ABS(glsl::length(end - start), 0.0f, 1e-5f, ());
  TEST_ALMOST_EQUAL_ABS(end.x, 5.0f, 1e-5f, ());
  TEST_ALMOST_EQUAL_ABS(end.y, -5.0f, 1e-5f, ());
}

UNIT_TEST(ClipSplinesBuilder_ScreenSpaceGap)
{
  for (float const density : {1.0f, 2.0f, 3.0f})
    for (float const roadWidth : {3.2f, 9.4f, 15.0f})
      for (int const side : {-1, 1})
      {
        float const road = roadWidth * density, line = 2.5f * density, gap = 2.0f * density;
        float const offset = df::SideOffsetPixels(road, line, gap, side);
        std::vector<P> const path{P(0, 0), P(10, 0)};
        auto const extrusion = df::LinePixelOffset(path, 0, offset);
        for (float const smoothZoom : {1.0f, 1.25f, 1.5f, 1.999f, 4.0f})
        {
          // The line shaders normalize the transformed normal and restore its
          // pixel length, independently of the map/model-view zoom scale.
          auto const transformed = extrusion * smoothZoom;
          auto const pixels = glsl::normalize(transformed) * glsl::length(extrusion);
          TEST_ALMOST_EQUAL_ABS(std::abs(pixels.y) - 0.5f * (road + line), gap, 1e-4f, (smoothZoom));
        }
        TEST_EQUAL(df::SideOffsetPixels(road, line, gap, 0), 0.0f, ());
      }
}

UNIT_TEST(ClipSplinesBuilder_ScreenSpaceMiter)
{
  std::vector<P> const corner{P(0, 0), P(10, 0), P(10, 10)};
  auto const right = df::LinePixelOffset(corner, 1, 5);
  auto const left = df::LinePixelOffset(corner, 1, -5);
  TEST_ALMOST_EQUAL_ABS(right.x, 5.0f, 1e-5f, ());
  TEST_ALMOST_EQUAL_ABS(right.y, -5.0f, 1e-5f, ());
  TEST_ALMOST_EQUAL_ABS(glsl::length(left + right), 0.0f, 1e-5f, ());
  std::vector<P> const reverse{P(0, 0), P(10, 0), P(0, 0.01)};
  auto const bounded = df::LinePixelOffset(reverse, 1, 5);
  TEST(std::isfinite(bounded.x) && std::isfinite(bounded.y), ());
  TEST_LESS_OR_EQUAL(glsl::length(bounded), 20.0f, ());
  TEST_ALMOST_EQUAL_ABS(glsl::length(df::LinePixelOffset(corner, 1, 0)), 0.0f, 1e-5f, ());
}

UNIT_TEST(ClipSplinesBuilder_ScreenSpaceArrowReversal)
{
  std::vector<P> const forward{P(0, 0), P(10, 0)};
  std::vector<P> const backward{P(10, 0), P(0, 0)};
  for (float const offset : {-10.0f, 10.0f})
  {
    auto const a = df::LinePixelOffset(forward, 0, offset);
    auto const b = df::LinePixelOffset(backward, 0, -offset);
    TEST_ALMOST_EQUAL_ABS(glsl::length(a - b), 0.0f, 1e-5f, ());
  }
}

UNIT_TEST(ClipSplinesBuilder_SideOffsetPadding)
{
  auto params = MakeParams(kNoSimplifyZoom, kMinSqr);
  params.m_tileRect = m2::RectD(0, 0, 10, 10);
  MockTestFeature feature({P(-2, -1), P(12, -1)});
  df::ClipSplinesBuilder builder(params);
  builder.Build(feature, kNoSimplifyZoom);
  TEST(!builder.HasGeometry(), ());
  builder.Build(feature, kNoSimplifyZoom, false, 2.0);
  TEST(builder.HasGeometry(), ());
  m2::SharedSpline source(builder.GetPath());
  auto const shifted = source.Equidistant(-2.0);
  auto const pieces = m2::ClipSplineByRect(params.m_tileRect, shifted);
  TEST_EQUAL(pieces.size(), 1, ());
  TEST_EQUAL(pieces.front()->GetPath().front(), P(0, 1), ());
  TEST_EQUAL(pieces.front()->GetPath().back(), P(10, 1), ());
}

UNIT_TEST(ClipSplinesBuilder_SideOffsetReversal)
{
  m2::SharedSpline forward(std::vector<P>{P(0, 0), P(10, 0)});
  m2::SharedSpline reversed(std::vector<P>{P(10, 0), P(0, 0)});
  auto const left = forward.Equidistant(-2.0);
  auto const reversedRight = reversed.Equidistant(2.0);
  TEST_EQUAL(left->GetPath().front(), P(0, 2), ());
  TEST_EQUAL(left->GetPath().front(), reversedRight->GetPath().back(), ());
  TEST_EQUAL(left->GetPath().back(), reversedRight->GetPath().front(), ());
}

UNIT_TEST(ClipSplinesBuilder_SinglePoint)
{
  auto params = MakeParams(kSimplifyZoom, kMinSqr);
  df::ClipSplinesBuilder builder(params);

  MockTestFeature f({P(5, 5)});
  builder.Build(f, kSimplifyZoom);

  TEST(!builder.HasGeometry(), ());
  TEST_EQUAL(builder.GetPathSize(), 1, ());
}

UNIT_TEST(ClipSplinesBuilder_NoSimplify_PreservesAllPoints)
{
  auto params = MakeParams(kNoSimplifyZoom, kMinSqr);
  df::ClipSplinesBuilder builder(params);

  // Even very close points are kept verbatim when simplification is off.
  std::vector<P> const input{P(0, 0), P(0.1, 0), P(0.2, 0), P(0.3, 0), P(10, 0)};
  MockTestFeature f(input);
  builder.Build(f, kNoSimplifyZoom);

  TestPath(builder, input);
}

UNIT_TEST(ClipSplinesBuilder_NoSimplify_DropsExactDuplicates)
{
  auto params = MakeParams(kNoSimplifyZoom, kMinSqr);
  df::ClipSplinesBuilder builder(params);

  // Consecutive duplicates collapse (zero-length segments are never useful).
  MockTestFeature f({P(0, 0), P(0, 0), P(5, 0), P(5, 0), P(10, 0)});
  builder.Build(f, kNoSimplifyZoom);

  TestPath(builder, {P(0, 0), P(5, 0), P(10, 0)});
}

UNIT_TEST(ClipSplinesBuilder_Simplify_FarApart_AllKept)
{
  auto params = MakeParams(kSimplifyZoom, kMinSqr);
  df::ClipSplinesBuilder builder(params);

  // All segments are far above sqrt(minSqr)=1.0 and the path zigzags
  // (90° turns) so colinearity never collapses anything.
  std::vector<P> const input{P(0, 0), P(10, 0), P(10, 10), P(20, 10)};
  MockTestFeature f(input);
  builder.Build(f, kSimplifyZoom);

  TestPath(builder, input);
}

UNIT_TEST(ClipSplinesBuilder_Simplify_DenseStart)
{
  // Three close points at the start of the feature, then a far point.
  // OLD algorithm produced [B1, Bn, P] (with a tiny B1->Bn opening segment).
  // NEW algorithm thins the dense cluster down to its first member and
  // jumps straight to the far point: [B1, P]. The endpoint coordinate is
  // the actual feature endpoint P.
  auto params = MakeParams(kSimplifyZoom, kMinSqr);
  df::ClipSplinesBuilder builder(params);

  MockTestFeature f({P(0, 0), P(0.2, 0), P(0.4, 0), P(0.6, 0), P(10, 0)});
  builder.Build(f, kSimplifyZoom);

  TestPath(builder, {P(0, 0), P(10, 0)});
}

UNIT_TEST(ClipSplinesBuilder_Simplify_MidFeatureDense)
{
  // Dense cluster sandwiched between far points. NEW behavior: keeps the
  // first point we entered the cluster with (P1) and drops the cluster.
  // (OLD behavior would drift the kept vertex to Bn.)
  auto params = MakeParams(kSimplifyZoom, kMinSqr);
  df::ClipSplinesBuilder builder(params);

  MockTestFeature f({P(0, 0), P(10, 0), P(10.2, 0.1), P(10.4, 0.2), P(10.6, 0.3), P(20, 5)});
  builder.Build(f, kSimplifyZoom);

  // Note: the segment from (10,0) to (20,5) is non-colinear with (0,0)->(10,0),
  // so the colinearity collapse does not fire.
  TestPath(builder, {P(0, 0), P(10, 0), P(20, 5)});
}

UNIT_TEST(ClipSplinesBuilder_Simplify_DenseEnd_PreservesEndpoint)
{
  // Dense cluster at the end of the feature. The algorithm thins the cluster
  // but the actual endpoint coordinate (the very last input point) is
  // preserved by replacing the kept tail with it — no tiny tail segment.
  auto params = MakeParams(kSimplifyZoom, kMinSqr);
  df::ClipSplinesBuilder builder(params);

  MockTestFeature f({P(0, 0), P(10, 0), P(10.2, 0.1), P(10.4, 0.2), P(10.6, 0.3)});
  builder.Build(f, kSimplifyZoom);

  // Last point is replaced into the kept tail, NOT appended after (10,0).
  TestPath(builder, {P(0, 0), P(10.6, 0.3)});
}

UNIT_TEST(ClipSplinesBuilder_Simplify_AllDense_TwoPointsKept)
{
  // Pathological: every input point is within minSqr of the previous one.
  // First point is always kept; last input point is preserved as the
  // endpoint via the post-loop append (m_path.size() == 1 path).
  auto params = MakeParams(kSimplifyZoom, kMinSqr);
  df::ClipSplinesBuilder builder(params);

  MockTestFeature f({P(0, 0), P(0.2, 0), P(0.4, 0), P(0.6, 0)});
  builder.Build(f, kSimplifyZoom);

  TestPath(builder, {P(0, 0), P(0.6, 0)});
}

UNIT_TEST(ClipSplinesBuilder_Simplify_Colinear_Collapses)
{
  // Forward-colinear stretch: each consecutive segment continues along the
  // same direction within the 0.995 cosine tolerance. Intermediate vertices
  // are replaced (not appended), collapsing the run to its endpoints.
  auto params = MakeParams(kSimplifyZoom, kMinSqr);
  df::ClipSplinesBuilder builder(params);

  MockTestFeature f({P(0, 0), P(5, 0), P(10, 0), P(15, 0), P(20, 0)});
  builder.Build(f, kSimplifyZoom);

  TestPath(builder, {P(0, 0), P(20, 0)});
}

UNIT_TEST(ClipSplinesBuilder_Simplify_UTurn_NotCollapsed)
{
  // [A, B, A] U-turn: the second segment is colinear-but-backward. With a
  // forward-only colinearity check (no fabs), the backward segment is
  // pushed normally, preserving the U-turn instead of degenerating to
  // [A, A] or dropping the feature entirely.
  auto params = MakeParams(kSimplifyZoom, kMinSqr);
  df::ClipSplinesBuilder builder(params);

  MockTestFeature f({P(0, 0), P(10, 0), P(0, 0)});
  builder.Build(f, kSimplifyZoom);

  TestPath(builder, {P(0, 0), P(10, 0), P(0, 0)});
}

UNIT_TEST(ClipSplinesBuilder_Simplify_DropsExactDuplicates)
{
  // Same as the no-simplify duplicate test, but with simplification on.
  // The exact duplicates are dropped by the IsAlmostZero check; the 90°
  // turn at (5,0) is not colinear (cos ≈ 0.707 < 0.995) so (5,0) is kept.
  auto params = MakeParams(kSimplifyZoom, kMinSqr);
  df::ClipSplinesBuilder builder(params);

  MockTestFeature f({P(0, 0), P(0, 0), P(5, 0), P(5, 0), P(10, 5)});
  builder.Build(f, kSimplifyZoom);

  TestPath(builder, {P(0, 0), P(5, 0), P(10, 5)});
}

UNIT_TEST(ClipSplinesBuilder_LimitRect_Outside)
{
  // Feature lies entirely outside the tile rect. Build should short-circuit
  // before reading any points; m_path stays empty.
  df::ApplyFeatureParams params;
  params.m_tileKey = df::TileKey(0, 0, kSimplifyZoom);
  params.m_tileRect = m2::RectD(0, 0, 10, 10);
  params.m_minSegmentSqrLength = kMinSqr;

  df::ClipSplinesBuilder builder(params);

  MockTestFeature f({P(100, 100), P(110, 110), P(120, 120)});
  builder.Build(f, kSimplifyZoom);

  TEST(!builder.HasGeometry(), ());
  TEST_EQUAL(builder.GetPathSize(), 0, ());
  TEST(!builder.IsKnownInside(), ());
}

UNIT_TEST(ClipSplinesBuilder_LimitRect_Inside)
{
  // Feature lies entirely inside the tile rect. Build should set the
  // Inside hint so Release() can take the fast path.
  df::ApplyFeatureParams params;
  params.m_tileKey = df::TileKey(0, 0, kSimplifyZoom);
  params.m_tileRect = m2::RectD(-100, -100, 100, 100);
  params.m_minSegmentSqrLength = kMinSqr;

  df::ClipSplinesBuilder builder(params);

  MockTestFeature f({P(0, 0), P(20, 0), P(20, 20)});
  builder.Build(f, kSimplifyZoom);

  TEST(builder.HasGeometry(), ());
  TEST(builder.IsKnownInside(), ());
  TestPath(builder, {P(0, 0), P(20, 0), P(20, 20)});
}

UNIT_TEST(ClipSplinesBuilder_LimitRect_Crossing)
{
  // Feature's limit rect overlaps the tile rect but isn't contained — the
  // hint stays Unknown and the path is read in full (clipping itself
  // happens later in Release()).
  df::ApplyFeatureParams params;
  params.m_tileKey = df::TileKey(0, 0, kSimplifyZoom);
  params.m_tileRect = m2::RectD(0, 0, 10, 10);
  params.m_minSegmentSqrLength = kMinSqr;

  df::ClipSplinesBuilder builder(params);

  MockTestFeature f({P(-5, 5), P(5, 5), P(15, 5)});
  builder.Build(f, kSimplifyZoom);

  TEST(builder.HasGeometry(), ());
  TEST(!builder.IsKnownInside(), ());
  // (-5,5)->(5,5)->(15,5) is a colinear forward run; the middle point is
  // collapsed by the colinearity replace.
  TestPath(builder, {P(-5, 5), P(15, 5)});
}

UNIT_TEST(ClipSplinesBuilder_ClippedSplineOffset)
{
  df::ApplyFeatureParams params;
  params.m_tileKey = df::TileKey(0, 0, kNoSimplifyZoom);
  params.m_tileRect = m2::RectD(5, -1, 15, 1);
  params.m_minSegmentSqrLength = kMinSqr;

  df::ClipSplinesBuilder builder(params);
  MockTestFeature f({P(0, 0), P(10, 0), P(20, 0)});
  builder.Build(f, kNoSimplifyZoom);

  auto const splines = builder.Release(false /* isIsoline */, true /* needSplineOffsets */);
  auto const & offsets = builder.GetSplineOffsets();
  TEST_EQUAL(splines.size(), 1, ());
  TEST_EQUAL(offsets.size(), splines.size(), ());
  TEST_ALMOST_EQUAL_ABS(offsets.front(), 5.0, 1e-12, ());
}

UNIT_TEST(DashedLinePhase_ReversedMetalineMember)
{
  double constexpr kFeatureLength = 10.0;
  double const forwardEnd = df::AdvanceDashPhase(0.0, kFeatureLength, false /* reversed */);
  double const reversedEnd = df::AdvanceDashPhase(2.0 * kFeatureLength, kFeatureLength, true /* reversed */);

  // Both features meet at the same chain offset even though the second feature's
  // source geometry runs in the opposite direction.
  TEST_ALMOST_EQUAL_ABS(df::WrapDashPhase(forwardEnd, 7.0), df::WrapDashPhase(reversedEnd, 7.0), 1e-12, ());
}

UNIT_TEST(DashedLinePhase_LargeOffsetIsWrappedBeforeFloatConversion)
{
  double constexpr kMaskLength = 31.0;
  double constexpr kLargeOffset = 1.0e12 + 17.25;
  double const wrapped = df::WrapDashPhase(kLargeOffset, kMaskLength);

  TEST_GREATER_OR_EQUAL(wrapped, 0.0, ());
  TEST_LESS(wrapped, kMaskLength, ());
  TEST_ALMOST_EQUAL_ABS(wrapped, std::fmod(kLargeOffset, kMaskLength), 1e-12, ());
  TEST_ALMOST_EQUAL_ABS(static_cast<double>(static_cast<float>(wrapped)), wrapped, 1e-5, ());
}

UNIT_TEST(ClipSplinesBuilder_LimitRect_Isoline_InSmoothBand)
{
  // Isoline feature whose limit rect is strictly outside tileRect but inside
  // tileRect.Scale(kIsolineSmoothScale). Must NOT be short-circuited — the isoline smoother
  // needs the full control-point set to avoid seams at tile boundaries.
  df::ApplyFeatureParams params;
  params.m_tileKey = df::TileKey(0, 0, kSimplifyZoom);
  params.m_tileRect = m2::RectD(0, 0, 10, 10);  // extTileRect = (-3, -3, 13, 13)
  params.m_minSegmentSqrLength = kMinSqr;

  df::ClipSplinesBuilder builder(params);

  MockTestFeature f({P(11, 5), P(12, 5), P(12, 6)});
  builder.Build(f, kSimplifyZoom, true /* isIsoline */);

  TEST(builder.HasGeometry(), ());
  TEST(!builder.IsKnownInside(), ());  // never set for isolines
}
}  // namespace clip_spline_builder_tests
