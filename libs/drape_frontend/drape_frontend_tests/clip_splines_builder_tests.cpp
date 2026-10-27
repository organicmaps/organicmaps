#include "testing/testing.hpp"

#include "drape_frontend/apply_feature_params.hpp"
#include "drape_frontend/clip_splines_builder.hpp"
#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/tile_key.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/terrain/isolines_tracer.hpp"
#include "indexer/terrain/terrain_reader.hpp"

#include "platform/platform.hpp"

#include "coding/point_coding.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

// Friend of FeatureType (declared in indexer/feature.hpp). Lets a test set up
// a fully-formed line feature without going through a real .mwm load.
/// @todo Factor out as a reusable class into some xxx_test_support.
class MockTestFeature : public FeatureType
{
public:
  explicit MockTestFeature(std::vector<m2::PointD> const & points) : FeatureType(FeatureID{}, 0)
  {
    m_header = static_cast<uint8_t>(feature::HeaderGeomType::Line);
    m_points.assign(points.begin(), points.end());
    m_limitRect.MakeEmpty();
    feature::CalcRect(m_points, m_limitRect);
    m_parsed.m_points = true;
    m_parsed.m_triangles = true;
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
UNIT_TEST(ClipSplinesBuilder_SetPath_ThinsLikeBuild)
{
  // The dynamic isolines go through SetPath: on the simplified zooms they get exactly the
  // thinning of the baked ones (short segments, colinear runs, the endpoint and the ring).
  std::vector<std::vector<P>> const paths = {
      {P(0, 0), P(0.3, 0), P(0.6, 0.1), P(5, 0), P(10, 0.01), P(10, 5), P(10.2, 5.1)},
      {P(0, 0), P(10, 0), P(10, 10), P(0, 10), P(0, 0.5), P(0, 0)},
      {P(0, 0), P(10, 0), P(5, 0.1), P(5, 0.2)},
  };
  for (auto const & points : paths)
  {
    auto const params = MakeParams(kSimplifyZoom, kMinSqr);
    df::ClipSplinesBuilder built(params);
    MockTestFeature f(points);
    built.Build(f, kSimplifyZoom);

    df::ClipSplinesBuilder set(params);
    set.SetPath(std::vector<P>(points));
    TestPath(set, built.GetPath());
    TEST_LESS(set.GetPathSize(), points.size(), ());
  }

  // The other zooms keep the dynamic path as is.
  auto const params = MakeParams(kNoSimplifyZoom, kMinSqr);
  df::ClipSplinesBuilder builder(params);
  builder.SetPath(std::vector<P>(paths.front()));
  TestPath(builder, paths.front());
}

UNIT_TEST(ClipSplinesBuilder_SetPath_AveragesRasterSteps)
{
  // A contour stepping along the 1 arc second DEM cells near the equator: its steps flatten into a
  // line and its ends stay, while a coarse (TIN) path keeps its corners.
  double constexpr kCell = 1.0 / 3600;
  std::vector<P> steps;
  for (int i = 0; i < 8; ++i)
    steps.insert(steps.end(), {P(i * kCell, i * kCell), P((i + 1) * kCell, i * kCell)});
  steps.emplace_back(8 * kCell, 8 * kCell);

  auto const params = MakeParams(kNoSimplifyZoom, kMinSqr);
  df::ClipSplinesBuilder builder(params);
  builder.SetPath(std::vector<P>(steps));
  auto const & path = builder.GetPath();
  TEST_EQUAL(path.size(), steps.size(), ());
  TEST_EQUAL(path.front(), steps.front(), ());
  TEST_EQUAL(path.back(), steps.back(), ());
  for (size_t i = 2; i + 2 < path.size(); ++i)
    TEST_ALMOST_EQUAL_ABS(path[i].x - path[i].y, kCell / 2, 1e-12, (i, path));

  std::vector<P> const coarse = {P(0, 0), P(100 * kCell, 0), P(100 * kCell, 100 * kCell)};
  builder.SetPath(std::vector<P>(coarse));
  TestPath(builder, coarse);
}

// The thinned dynamic isolines of two neighbor tiles meet on their shared edge: each tile traces
// its own inflated rect, so the chains start elsewhere, but the crossings must stay identical.
using df::test_support::VisualParamsFixture;

UNIT_CLASS_TEST(VisualParamsFixture, ClipSplinesBuilder_DynamicIsolinesTileSeams)
{
  terrain::Reader const reader(base::JoinPath(GetPlatform().ResourcesDir(), "N43E007.twm"));
  auto const & header = reader.GetHeader();
  auto const center = header.GetLimitRect().Center();
  for (uint8_t const zoom : {10, 11, 12})
  {
    double const tileSize = mercator::Bounds::kRangeX / (1 << (zoom - 1));
    int const x = static_cast<int>(std::floor(center.x / tileSize));
    int const y = static_cast<int>(std::floor(center.y / tileSize));
    double const edgeX = (x + 1) * tileSize;

    auto const crossings = [&](df::TileKey const & key)
    {
      df::ApplyFeatureParams params;
      params.Init(key);
      TEST(params.IsSimplifyLines(), (zoom));
      // The same read as RuleDrawer::DrawTerrain and TerrainProvider::ReadTerrain.
      auto query = params.m_tileRect;
      query.Scale(df::kIsolineSmoothScale);
      size_t const geomIndex = header.GetGeometryIndex(zoom);
      if (header.HasRaster(geomIndex))
        query.Inflate(reader.GetRasterHalo().x, reader.GetRasterHalo().y);
      terrain::TerrainData data(header.m_coordBits);
      reader.ReadTerrain(query, geomIndex, data);

      std::vector<std::pair<int32_t, double>> result;
      df::ClipSplinesBuilder builder(params);
      terrain::TraceIsolines(data, 100, measurement_utils::Units::Metric, [&](terrain::Isoline && isoline)
      {
        builder.SetPath(std::move(isoline.m_points));
        for (auto const & spline : builder.Release(true /* isIsoline */))
          for (auto const & p : {spline->GetPath().front(), spline->GetPath().back()})
            if (std::abs(p.x - edgeX) < 1e-9)
              result.emplace_back(isoline.m_altitude, p.y);
      });
      std::sort(result.begin(), result.end());
      return result;
    };
    auto const left = crossings(df::TileKey(x, y, zoom));
    TEST(!left.empty(), (zoom));
    TEST_EQUAL(left, crossings(df::TileKey(x + 1, y, zoom)), (zoom));
  }
}

// The same at the high zooms, where the contours step along the 1 arc second raster and their
// vertices are averaged (see SetPath): the reader crops the raster patches to the read rect, so the
// chains end near it, and the averaging must not reach the tile from there (4 passes would).
UNIT_CLASS_TEST(VisualParamsFixture, ClipSplinesBuilder_DynamicIsolinesRasterStepsSeams)
{
  double constexpr kCell = 1.0 / 3600;
  ms::LatLon const origin(69.6, 18.9);
  // Integer terraces of a gentle (a few %) relief, a pure function of the global lattice node.
  auto const height = [](int64_t row, int64_t col)
  {
    return static_cast<terrain::Altitude>(
        std::lround(200 + 0.2 * col + 0.5 * row + 3 * std::sin(col / 13.0) + 3 * std::cos(row / 11.0)));
  };
  // The nodes of the lattice cells intersecting the rect, like terrain::Reader::ReadTerrain crops them.
  auto const read = [&](m2::RectD const & rect)
  {
    auto const c0 = static_cast<int64_t>(std::floor((rect.minX() - origin.m_lon) / kCell));
    auto const c1 = static_cast<int64_t>(std::ceil((rect.maxX() - origin.m_lon) / kCell));
    auto const r0 = static_cast<int64_t>(std::floor((mercator::YToLat(rect.minY()) - origin.m_lat) / kCell));
    auto const r1 = static_cast<int64_t>(std::ceil((mercator::YToLat(rect.maxY()) - origin.m_lat) / kCell));
    terrain::RasterPatch patch;
    for (auto c = c0; c <= c1; ++c)
      patch.m_columns.push_back(PointDToPointU(origin.m_lon + c * kCell, 0, patch.m_coordBits).x);
    for (auto r = r0; r <= r1; ++r)
      patch.m_rows.push_back(PointDToPointU(0, mercator::LatToY(origin.m_lat + r * kCell), patch.m_coordBits).y);
    for (auto r = r0; r <= r1; ++r)
      for (auto c = c0; c <= c1; ++c)
        patch.m_heights.push_back(height(r, c));
    terrain::TerrainData data;
    data.m_rasterPatches.push_back(std::move(patch));
    return data;
  };

  auto const center = mercator::FromLatLon(69.65, 18.95);
  // The clipping may stop a spline a few millimeters short of the tile edge.
  double constexpr kEps = 1e-7;
  for (uint8_t const zoom : {16, 17})
  {
    double const tileSize = mercator::Bounds::kRangeX / (1 << (zoom - 1));
    // The isoline crossings of the tile edge x = edge (vertical) or y = edge.
    auto const crossings = [&](df::TileKey const & key, bool vertical, double edge)
    {
      df::ApplyFeatureParams params;
      params.Init(key);
      // The same read as RuleDrawer::DrawTerrain, the provider inflates it by one raster cell.
      auto query = params.m_tileRect;
      query.Scale(df::kIsolineSmoothScale);
      query.Inflate(kCell, kCell / std::cos(math::DegToRad(origin.m_lat)));
      std::vector<std::pair<int32_t, double>> result;
      df::ClipSplinesBuilder builder(params);
      terrain::TraceIsolines(read(query), 1, measurement_utils::Units::Metric, [&](terrain::Isoline && isoline)
      {
        builder.SetPath(std::move(isoline.m_points));
        for (auto const & spline : builder.Release(true /* isIsoline */))
          for (auto const & p : {spline->GetPath().front(), spline->GetPath().back()})
            if (std::abs((vertical ? p.x : p.y) - edge) < kEps)
              result.emplace_back(isoline.m_altitude, vertical ? p.y : p.x);
      });
      std::sort(result.begin(), result.end());
      return result;
    };
    auto const testEqual = [&](auto const & lhs, auto const & rhs, int x, int y)
    {
      TEST_EQUAL(lhs.size(), rhs.size(), (zoom, x, y, lhs, rhs));
      for (size_t i = 0; i < lhs.size(); ++i)
      {
        TEST_EQUAL(lhs[i].first, rhs[i].first, (zoom, x, y, lhs, rhs));
        TEST_ALMOST_EQUAL_ABS(lhs[i].second, rhs[i].second, kEps, (zoom, x, y, lhs, rhs));
      }
    };

    int const x0 = static_cast<int>(std::floor(center.x / tileSize));
    int const y0 = static_cast<int>(std::floor(center.y / tileSize));
    size_t count = 0;
    for (int x = x0; x < x0 + 8; ++x)
    {
      for (int y = y0; y < y0 + 8; ++y)
      {
        auto const right = crossings(df::TileKey(x, y, zoom), true /* vertical */, (x + 1) * tileSize);
        testEqual(right, crossings(df::TileKey(x + 1, y, zoom), true /* vertical */, (x + 1) * tileSize), x, y);
        auto const top = crossings(df::TileKey(x, y, zoom), false /* vertical */, (y + 1) * tileSize);
        testEqual(top, crossings(df::TileKey(x, y + 1, zoom), false /* vertical */, (y + 1) * tileSize), x, y);
        count += right.size() + top.size();
      }
    }
    TEST_GREATER(count, 200, (zoom));
  }
}

}  // namespace clip_spline_builder_tests
