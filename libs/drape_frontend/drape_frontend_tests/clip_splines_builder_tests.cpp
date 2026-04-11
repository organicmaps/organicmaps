#include "testing/testing.hpp"

#include "drape_frontend/apply_feature_params.hpp"
#include "drape_frontend/clip_splines_builder.hpp"
#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_data.hpp"

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
  df::VisualParams::Init(1.0, 1024);

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
  df::VisualParams::Init(1.0, 1024);

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
  df::VisualParams::Init(1.0, 1024);

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
  df::VisualParams::Init(1.0, 1024);

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
  df::VisualParams::Init(1.0, 1024);

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
