#include "testing/testing.hpp"

#include "drape_frontend/navigator.hpp"
#include "drape_frontend/screen_operations.hpp"
#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/visual_params.hpp"

#include "geometry/mercator.hpp"
#include "geometry/screenbase.hpp"
#include "geometry/transformations.hpp"

#include "base/math.hpp"

namespace screen_operations_tests
{

// -- WrapTileX ----------------------------------------------------------------

UNIT_TEST(WrapTileX_Zoom1)
{
  // Zoom 1: numTiles = 1, minX = -((1+1)/2) = -1, canonical range = [-1, 0).
  df::TileKey canonical(-1, 0, 1);
  TEST_EQUAL(canonical.GetCanonicalTileKey().m_x, -1, ());

  df::TileKey east(0, 0, 1);
  TEST_EQUAL(east.GetCanonicalTileKey().m_x, -1, ());

  df::TileKey west(-2, 0, 1);
  TEST_EQUAL(west.GetCanonicalTileKey().m_x, -1, ());
}

UNIT_TEST(WrapTileX_Zoom2)
{
  // Zoom 2: numTiles = 2, canonical range = [-1, 1), minX = -1.
  df::TileKey t0(-1, 0, 2);
  TEST_EQUAL(t0.GetCanonicalTileKey().m_x, -1, ());

  df::TileKey t1(0, 0, 2);
  TEST_EQUAL(t1.GetCanonicalTileKey().m_x, 0, ());

  // X=1 wraps to -1
  df::TileKey east(1, 0, 2);
  TEST_EQUAL(east.GetCanonicalTileKey().m_x, -1, ());

  // X=-2 wraps to 0
  df::TileKey west(-2, 0, 2);
  TEST_EQUAL(west.GetCanonicalTileKey().m_x, 0, ());
}

UNIT_TEST(WrapTileX_Zoom3)
{
  // Zoom 3: numTiles = 4, canonical range = [-2, 2), minX = -2.
  df::TileKey t(2, 0, 3);
  TEST_EQUAL(t.GetCanonicalTileKey().m_x, -2, ());

  df::TileKey t2(5, 0, 3);
  TEST_EQUAL(t2.GetCanonicalTileKey().m_x, 1, ());

  df::TileKey t3(-3, 0, 3);
  TEST_EQUAL(t3.GetCanonicalTileKey().m_x, 1, ());
}

// -- GetWrappedDataRect -------------------------------------------------------

UNIT_TEST(GetWrappedDataRect_CanonicalTile)
{
  // A canonical tile's wrapped rect should equal its global rect.
  df::TileKey key(0, 0, 3);
  m2::RectD const global = key.GetGlobalRect();
  m2::RectD const wrapped = key.GetWrappedDataRect();
  TEST(global == wrapped, (global, wrapped));
}

UNIT_TEST(GetWrappedDataRect_ExtendedTile)
{
  // An extended tile (past +180) should produce a wrapped rect in [-180, 180].
  // At zoom 3, numTiles = 4, tileSize = 90°.
  // Tile X=2 has global rect starting at 180 — entirely outside canonical range.
  df::TileKey key(2, 0, 3);
  m2::RectD const wrapped = key.GetWrappedDataRect();
  TEST_LESS(wrapped.minX(), mercator::Bounds::kMaxX, ());
  TEST_GREATER(wrapped.maxX(), mercator::Bounds::kMinX, ());
}

UNIT_TEST(GetWrappedDataRect_OverlappingTile)
{
  // A tile that overlaps the canonical range should be returned as-is.
  // At zoom 3, tile X=1 covers [90, 180] — overlaps canonical range.
  df::TileKey key(1, 0, 3);
  m2::RectD const global = key.GetGlobalRect();
  m2::RectD const wrapped = key.GetWrappedDataRect();
  TEST(global == wrapped, (global, wrapped));
}

// -- GetTileXOffset -----------------------------------------------------------

UNIT_TEST(GetTileXOffset_CanonicalIsZero)
{
  df::TileKey key(0, 0, 3);
  TEST_ALMOST_EQUAL_ABS(key.GetTileXOffset(), 0.0, 1e-10, ());
}

UNIT_TEST(GetTileXOffset_ExtendedTile)
{
  // Extended tile should have non-zero offset (approximately +-360).
  df::TileKey key(2, 0, 3);
  double const offset = key.GetTileXOffset();
  TEST_ALMOST_EQUAL_ABS(std::abs(offset), 360.0, 1.0, ());
}

// -- GetCanonicalTileKey ------------------------------------------------------

UNIT_TEST(GetCanonicalTileKey_Preserves_Y_and_Zoom)
{
  df::TileKey key(5, 3, 4);
  df::TileKey canonical = key.GetCanonicalTileKey();
  TEST_EQUAL(canonical.m_y, key.m_y, ());
  TEST_EQUAL(canonical.m_zoomLevel, key.m_zoomLevel, ());
}

// -- AdjustPointForViewport ---------------------------------------------------

UNIT_TEST(AdjustPointForViewport_NoWrap)
{
  // Screen origin at 0 — point at 10 is within 180, no adjustment.
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-10, -10, 10, 10)));

  m2::PointD const result = df::AdjustPointForViewport({10.0, 5.0}, screen);
  TEST_ALMOST_EQUAL_ABS(result.x, 10.0, 1e-10, ());
  TEST_ALMOST_EQUAL_ABS(result.y, 5.0, 1e-10, ());
}

UNIT_TEST(AdjustPointForViewport_WrapEast)
{
  // Screen origin at -170 — point at 170 is > 180 away, should wrap to -190.
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-180, -10, -160, 10)));

  m2::PointD const result = df::AdjustPointForViewport({170.0, 5.0}, screen);
  TEST_ALMOST_EQUAL_ABS(result.x, -190.0, 1e-10, ());
}

UNIT_TEST(AdjustPointForViewport_WrapWest)
{
  // Screen origin at 170 — point at -170 is > 180 away, should wrap to 190.
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(160, -10, 180, 10)));

  m2::PointD const result = df::AdjustPointForViewport({-170.0, 5.0}, screen);
  TEST_ALMOST_EQUAL_ABS(result.x, 190.0, 1e-10, ());
}

// -- NormalizeScreenOriginX ---------------------------------------------------

UNIT_TEST(NormalizeScreenOriginX_WithinRange)
{
  // Origin within [-540, 540] — no change.
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-10, -10, 10, 10)));
  m2::PointD const orgBefore = screen.GetOrg();

  df::NormalizeScreenOriginX(screen);
  TEST_ALMOST_EQUAL_ABS(screen.GetOrg().x, orgBefore.x, 1e-10, ());
}

UNIT_TEST(NormalizeScreenOriginX_WrapPositive)
{
  // Origin at 600 (> 540) — should wrap into [-540, 540].
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(590, -10, 610, 10)));

  df::NormalizeScreenOriginX(screen);
  double const x = screen.GetOrg().x;
  TEST_LESS_OR_EQUAL(x, 540.0, ());
  TEST_GREATER_OR_EQUAL(x, -540.0, ());
  // Should be equivalent modulo 360.
  TEST_ALMOST_EQUAL_ABS(std::fmod(x - 600.0 + 3600.0, 360.0), 0.0, 1e-10, ());
}

UNIT_TEST(NormalizeScreenOriginX_WrapNegative)
{
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-610, -10, -590, 10)));

  df::NormalizeScreenOriginX(screen);
  double const x = screen.GetOrg().x;
  TEST_LESS_OR_EQUAL(x, 540.0, ());
  TEST_GREATER_OR_EQUAL(x, -540.0, ());
}

// -- CheckMinScale ------------------------------------------------------------

UNIT_TEST(CheckMinScale_WithinBounds)
{
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-50, -30, 50, 30)));

  TEST(df::CheckMinScale(screen), ("Within world bounds should pass"));
}

UNIT_TEST(CheckMinScale_XExceeded)
{
  // Viewport wider than world in X — should fail (would show duplicate copies).
  ScreenBase screen;
  screen.OnSize(0, 0, 2000, 100);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-400, -50, 400, 50)));

  TEST(!df::CheckMinScale(screen), ("X wider than world should fail"));
}

UNIT_TEST(CheckMinScale_YExceeded)
{
  // Viewport exceeds world in Y — should fail.
  ScreenBase screen;
  screen.OnSize(0, 0, 100, 2000);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-50, -200, 50, 200)));

  TEST(!df::CheckMinScale(screen), ("Y exceeding world should fail"));
}

// -- CheckBorders -------------------------------------------------------------

UNIT_TEST(CheckBorders_YWithinBounds)
{
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  // Use a rect with matching aspect ratio so ClipRect Y stays within world bounds.
  screen.SetFromRect(m2::AnyRectD(m2::RectD(100, -75, 500, 75)));

  TEST(df::CheckBorders(screen), ("Y within bounds, X extended — should pass"));
}

UNIT_TEST(CheckBorders_YOutOfBounds)
{
  // Viewport Y extends past poles.
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-100, -200, 100, 200)));

  TEST(!df::CheckBorders(screen), ("Y exceeding poles should fail"));
}

// -- ShrinkInto (Y-only) ------------------------------------------------------

UNIT_TEST(ShrinkInto_ClampsY)
{
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  // Place screen with Y partially outside world rect (shifted north).
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-50, 100, 50, 200)));

  m2::RectD const & worldR = df::GetWorldRect();
  df::ShrinkInto(screen, worldR);

  m2::RectD const clip = screen.ClipRect();
  TEST_GREATER_OR_EQUAL(clip.minY(), worldR.minY(), ());
  TEST_LESS_OR_EQUAL(clip.maxY(), worldR.maxY(), ());
}

UNIT_TEST(ShrinkInto_DoesNotClampX)
{
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  // Place screen with X far outside canonical range.
  screen.SetFromRect(m2::AnyRectD(m2::RectD(500, -50, 600, 50)));

  double const orgXBefore = screen.GetOrg().x;
  df::ShrinkInto(screen, df::GetWorldRect());

  // X should be unchanged (no X clamping).
  TEST_ALMOST_EQUAL_ABS(screen.GetOrg().x, orgXBefore, 1e-10, ());
}

// -- ScaleInto (Y-only) -------------------------------------------------------

UNIT_TEST(ScaleInto_ScalesForY)
{
  df::VisualParams::Init(1.0, 1024);

  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  // Screen with Y exceeding world bounds.
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-50, -250, 50, 250)));

  df::ScaleInto(screen, df::GetWorldRect());

  m2::RectD const clip = screen.ClipRect();
  m2::RectD const & worldR = df::GetWorldRect();
  TEST_LESS_OR_EQUAL(clip.maxY(), worldR.maxY() + 1e-5, ());
  TEST_GREATER_OR_EQUAL(clip.minY(), worldR.minY() - 1e-5, ());
}

UNIT_TEST(ScaleInto_ClampsXWidth)
{
  df::VisualParams::Init(1.0, 1024);

  ScreenBase screen;
  screen.OnSize(0, 0, 2000, 100);
  // Viewport wider than world; center off the canonical range to verify X
  // position is preserved (no panning).
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-300, -10, 100, 10)));
  double const orgXBefore = screen.GetOrg().x;

  df::ScaleInto(screen, df::GetWorldRect());

  m2::RectD const clip = screen.ClipRect();
  m2::RectD const & worldR = df::GetWorldRect();
  TEST_LESS_OR_EQUAL(clip.SizeX(), worldR.SizeX() + 1e-5, ());
  TEST_ALMOST_EQUAL_ABS(screen.GetOrg().x, orgXBefore, 1e-5, ());
}

// -- ShrinkAndScaleInto -------------------------------------------------------

UNIT_TEST(ShrinkAndScaleInto_ClampsXWidth)
{
  df::VisualParams::Init(1.0, 1024);

  ScreenBase screen;
  screen.OnSize(0, 0, 2000, 100);
  // Viewport wider than world.
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-400, -50, 400, 50)));

  df::ShrinkAndScaleInto(screen, df::GetWorldRect());

  m2::RectD const clip = screen.ClipRect();
  m2::RectD const & worldR = df::GetWorldRect();
  TEST_LESS_OR_EQUAL(clip.SizeX(), worldR.SizeX() + 1e-5, ());
}

UNIT_TEST(ShrinkAndScaleInto_ClampsY)
{
  df::VisualParams::Init(1.0, 1024);

  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-50, 100, 50, 200)));

  df::ShrinkAndScaleInto(screen, df::GetWorldRect());

  m2::RectD const clip = screen.ClipRect();
  m2::RectD const & worldR = df::GetWorldRect();
  TEST_LESS_OR_EQUAL(clip.maxY(), worldR.maxY() + 1e-5, ());
  TEST_GREATER_OR_EQUAL(clip.minY(), worldR.minY() - 1e-5, ());
}

// -- Navigator DoDrag (antimeridian wrapping) ----------------------------------

UNIT_TEST(Navigator_DoDrag_HorizontalUnbounded)
{
  df::VisualParams::Init(1.0, 1024);
  df::Navigator navigator;

  navigator.OnSize(800, 600);
  navigator.SetFromRect(m2::AnyRectD(m2::RectD(160, -10, 180, 10)));

  ScreenBase const & screen = navigator.Screen();
  m2::PointD const startPx = screen.GtoP(m2::PointD(170, 0));

  // Drag rightward — should cross antimeridian without clamping.
  navigator.StartDrag(startPx);
  navigator.DoDrag(m2::PointD(startPx.x - 100, startPx.y));
  navigator.StopDrag(m2::PointD(startPx.x - 100, startPx.y));

  // Screen origin X should have moved past 180.
  TEST_GREATER(navigator.Screen().GetOrg().x, 170.0, ());
}

UNIT_TEST(Navigator_SetFromRect_ClampsWideX)
{
  df::VisualParams::Init(1.0, 1024);
  df::Navigator navigator;
  navigator.OnSize(800, 600);

  // Programmatic SetFromRect with rect wider than the world.
  navigator.SetFromRect(m2::AnyRectD(m2::RectD(-400, -100, 400, 100)));

  m2::RectD const clip = navigator.Screen().ClipRect();
  m2::RectD const & worldR = df::GetWorldRect();
  TEST_LESS_OR_EQUAL(clip.SizeX(), worldR.SizeX() + 1e-5, ());
}

UNIT_TEST(Navigator_DoDrag_VerticalClamped)
{
  df::VisualParams::Init(1.0, 1024);
  df::Navigator navigator;

  navigator.OnSize(800, 600);
  navigator.SetFromRect(m2::AnyRectD(m2::RectD(-10, 160, 10, 180)));

  ScreenBase const & screen = navigator.Screen();
  m2::PointD const startPx = screen.GtoP(m2::PointD(0, 170));

  // Drag upward — should be clamped at poles.
  navigator.StartDrag(startPx);
  navigator.DoDrag(m2::PointD(startPx.x, startPx.y - 500));
  navigator.StopDrag(m2::PointD(startPx.x, startPx.y - 500));

  m2::RectD const clip = navigator.Screen().ClipRect();
  m2::RectD const & worldR = df::GetWorldRect();
  TEST_LESS_OR_EQUAL(clip.maxY(), worldR.maxY() + 1e-5, ());
}

// -- Navigator perspective gesture tracking -----------------------------------
// Verify that two-finger Scale and Rotate keep the geo point under each finger
// glued to that finger, including when auto-perspective recomputes
// m_3dAngleX / m_PixelRect mid-step.

namespace
{
// Build a navigator zoomed in enough for auto-perspective to engage.
df::Navigator MakePerspectiveNavigator(int w, int h)
{
  df::Navigator nav;
  nav.OnSize(w, h);

  // m_Scale = 1.25e-5 global/px → perspective angle ~14.5° (auto range is < 1.7e-5).
  double const perspectiveScale = nav.Screen().GetStartPerspectiveScale() * 0.6;

  double const hx = 0.5 * w * perspectiveScale;
  double const hy = 0.5 * h * perspectiveScale;
  nav.SetFromRect(m2::AnyRectD(m2::RectD(-hx, -hy, hx, hy)));
  nav.Enable3dMode();
  nav.SetAutoPerspective(true);

  TEST_GREATER(nav.Screen().GetRotationAngle(), math::DegToRad(10.0), ());
  TEST(nav.Screen().isPerspective(), ());
  return nav;
}

// Geo point currently under a finger at the given P3d position.
m2::PointD GeoUnderFinger(df::Navigator const & nav, m2::PointD const & p3d)
{
  return nav.PtoG(nav.P3dtoP(p3d));
}

bool PointsClose(m2::PointD const & a, m2::PointD const & b, double eps)
{
  return (a - b).Length() < eps;
}
}  // namespace

UNIT_TEST(Navigator_PerspectiveScale_KeepsFingersOnSameGeoPoints)
{
  df::VisualParams::Init(1.0, 1024);
  df::Navigator nav = MakePerspectiveNavigator(800, 600);

  // Two fingers on a horizontal line, centered near the middle of the screen.
  m2::PointD const pt1(300, 400);
  m2::PointD const pt2(500, 400);

  m2::PointD const g1Before = GeoUnderFinger(nav, pt1);
  m2::PointD const g2Before = GeoUnderFinger(nav, pt2);
  double const fingerSpan = (g2Before - g1Before).Length();
  TEST_GREATER(fingerSpan, 0.0, ());

  // Spread fingers (zoom in) by ~1.1x with no rotation. A small step keeps the
  // auto-perspective angle change small; large per-step jumps leave a
  // visible residual on the non-anchor finger which is unrealistic for real
  // gestures (touch events fire every ~10 ms).
  m2::PointD const newPt1(290, 400);
  m2::PointD const newPt2(510, 400);

  nav.StartScale(pt1, pt2);
  nav.DoScale(newPt1, newPt2);
  nav.StopScale(newPt1, newPt2);

  m2::PointD const g1After = GeoUnderFinger(nav, newPt1);
  m2::PointD const g2After = GeoUnderFinger(nav, newPt2);

  // Finger 1 is the explicit MatchGandP3d anchor — exact to numerical noise.
  TEST(PointsClose(g1After, g1Before, 1e-4 * fingerSpan), (g1Before, g1After));
  // Finger 2 tracks via the 2D similarity; the small auto-perspective shift
  // mid-step leaves a sub-3% residual on finger 2.
  TEST(PointsClose(g2After, g2Before, 0.03 * fingerSpan), (g2Before, g2After));
}

UNIT_TEST(Navigator_PerspectiveRotate_KeepsFingersOnSameGeoPoints)
{
  df::VisualParams::Init(1.0, 1024);
  df::Navigator nav = MakePerspectiveNavigator(800, 600);

  m2::PointD const screenCenter(400, 400);
  double const radius = 100.0;
  m2::PointD const pt1(screenCenter.x - radius, screenCenter.y);
  m2::PointD const pt2(screenCenter.x + radius, screenCenter.y);

  m2::PointD const g1Before = GeoUnderFinger(nav, pt1);
  m2::PointD const g2Before = GeoUnderFinger(nav, pt2);
  double const fingerSpan = (g2Before - g1Before).Length();
  TEST_GREATER(fingerSpan, 0.0, ());

  // Rotate the finger pair around its midpoint by 30° (above the 10° threshold
  // so DoScale flips IsRotatingDuringScale to true).
  auto const T = math::Shift(
      math::Rotate(math::Shift(math::Identity<double, 3>(), -screenCenter), math::DegToRad(30.0)), screenCenter);
  m2::PointD const newPt1 = pt1 * T;
  m2::PointD const newPt2 = pt2 * T;

  nav.StartScale(pt1, pt2);
  nav.DoScale(newPt1, newPt2);
  nav.StopScale(newPt1, newPt2);

  TEST(nav.IsRotatingDuringScale(), ("Rotation threshold should have tripped"));

  m2::PointD const g1After = GeoUnderFinger(nav, newPt1);
  m2::PointD const g2After = GeoUnderFinger(nav, newPt2);

  // Pure rotation keeps m_Scale stable, so perspective parameters don't move —
  // both fingers should anchor tightly.
  TEST(PointsClose(g1After, g1Before, 1e-4 * fingerSpan), (g1Before, g1After));
  TEST(PointsClose(g2After, g2Before, 0.02 * fingerSpan), (g2Before, g2After));
}

UNIT_TEST(Navigator_PerspectiveRotate_LargeAngle_TracksOffPivotFinger)
{
  df::VisualParams::Init(1.0, 1024);
  df::Navigator nav = MakePerspectiveNavigator(800, 600);

  // Vertical finger pair — the two fingers sit at very different perspective
  // depths, so the 2D similarity from CalcTransform cannot pin both. The test
  // requires per-finger P3dtoP plus the explicit MatchGandP3d anchor on finger 1.
  m2::PointD const screenCenter(400, 300);
  m2::PointD const pt1(screenCenter.x, screenCenter.y + 150);  // foreground
  m2::PointD const pt2(screenCenter.x, screenCenter.y - 150);  // toward horizon

  m2::PointD const g1Before = GeoUnderFinger(nav, pt1);
  m2::PointD const g2Before = GeoUnderFinger(nav, pt2);
  double const fingerSpan = (g2Before - g1Before).Length();
  TEST_GREATER(fingerSpan, 0.0, ());

  auto const T = math::Shift(
      math::Rotate(math::Shift(math::Identity<double, 3>(), -screenCenter), math::DegToRad(90.0)), screenCenter);
  m2::PointD const newPt1 = pt1 * T;
  m2::PointD const newPt2 = pt2 * T;

  nav.StartScale(pt1, pt2);
  nav.DoScale(newPt1, newPt2);
  nav.StopScale(newPt1, newPt2);

  TEST(nav.IsRotatingDuringScale(), ("Rotation threshold should have tripped"));

  m2::PointD const g1After = GeoUnderFinger(nav, newPt1);
  m2::PointD const g2After = GeoUnderFinger(nav, newPt2);

  // Finger 1 always anchored exactly.
  TEST(PointsClose(g1After, g1Before, 1e-4 * fingerSpan), (g1Before, g1After));
  // Finger 2 must stay close even though the two fingers straddle very different perspective depths.
  TEST(PointsClose(g2After, g2Before, 0.1 * fingerSpan), (g2Before, g2After));
}

UNIT_TEST(Navigator_PerspectiveScaleAndRotate_KeepsBothFingersAnchored)
{
  df::VisualParams::Init(1.0, 1024);
  df::Navigator nav = MakePerspectiveNavigator(800, 600);

  // Asymmetric pair: finger 2 noticeably above finger 1 (closer to the
  // horizon). The per-finger P3dtoP path matters here — a single linearizing
  // offset centered at finger 1 leaves a large drift on finger 2.
  m2::PointD const pt1(320, 500);
  m2::PointD const pt2(520, 200);

  m2::PointD const g1Before = GeoUnderFinger(nav, pt1);
  m2::PointD const g2Before = GeoUnderFinger(nav, pt2);
  double const fingerSpan = (g2Before - g1Before).Length();
  TEST_GREATER(fingerSpan, 0.0, ());

  // ~15° rotation + ~1.05x scale around the midpoint, both within the
  // rotation-threshold's scaling bound (1/1.2..1.2).
  double constexpr kScale = 1.05;
  m2::PointD const mid = (pt1 + pt2) / 2.0;
  auto const T = math::Shift(
      math::Scale(math::Rotate(math::Shift(math::Identity<double, 3>(), -mid), math::DegToRad(15.0)), kScale, kScale),
      mid);
  m2::PointD const newPt1 = pt1 * T;
  m2::PointD const newPt2 = pt2 * T;

  nav.StartScale(pt1, pt2);
  nav.DoScale(newPt1, newPt2);
  nav.StopScale(newPt1, newPt2);

  m2::PointD const g1After = GeoUnderFinger(nav, newPt1);
  m2::PointD const g2After = GeoUnderFinger(nav, newPt2);

  // Finger 1 is the MatchGandP3d anchor — exact to numerical noise.
  TEST(PointsClose(g1After, g1Before, 1e-4 * fingerSpan), (g1Before, g1After));
  // Finger 2 stays close because each touch is mapped through P3dtoP individually before CalcTransform.
  TEST(PointsClose(g2After, g2Before, 0.05 * fingerSpan), (g2Before, g2After));
}

}  // namespace screen_operations_tests
