#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/navigator.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/math.hpp"

#include <cmath>

namespace navigator_test
{
using df::test_support::VisualParamsFixture;

// -3 -2 -1  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
//
// 7      +-----------------------------------------------+
//        |                                               |
// 6      |                                               |
//        |                                               |
// 5      |                                               |
//        |                                               |
// 4      |  +-----------------------+                    |
//        |  |                       |                    |
// 3      |  |                       |                    |
//        |  |                       |                    |
// 2      |  |                       |                    |
//        |  |                       |                    |
// 1      |  |  a        B<-------b  |                    |
//        |  |                       |                    |
// 0      |  +-----------------------+                    |
//        |                                               |
//-1      +-----------------------------------------------+

UNIT_CLASS_TEST(VisualParamsFixture, Navigator_Scale2Points)
{
  df::Navigator navigator;

  navigator.OnSize(200, 100);
  navigator.SetFromRect(m2::AnyRectD(m2::RectD(0, 0, 8, 4)));

  ScreenBase const & screen = navigator.Screen();
  TEST_EQUAL(screen.ClipRect(), m2::RectD(0, 0, 8, 4), ());

  navigator.StartScale(screen.GtoP(m2::PointD(1, 1)), screen.GtoP(m2::PointD(7, 1)));
  navigator.StopScale(screen.GtoP(m2::PointD(1, 1)), screen.GtoP(m2::PointD(4, 1)));
  TEST_EQUAL(screen.ClipRect(), m2::RectD(-1, -1, 15, 7), ());
}

namespace
{
void CheckNavigator(df::Navigator const & nav)
{
  typedef m2::PointD P;
  m2::RectD clipR = nav.Screen().ClipRect();

  P arr[] = {clipR.LeftTop(), clipR.RightTop(), clipR.RightBottom(), clipR.LeftBottom(), clipR.Center()};

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
  {
    P const & pxP = arr[i];
    P const gP = nav.PtoG(pxP);
    P const pxP2 = nav.GtoP(gP);
    TEST(AlmostEqualAbs(pxP.x, pxP2.x, 0.00001), (pxP.x, pxP2.x));
    TEST(AlmostEqualAbs(pxP.y, pxP2.y, 0.00001), (pxP.y, pxP2.y));
  }
}
}  // namespace

UNIT_CLASS_TEST(VisualParamsFixture, Navigator_G2P_P2G)
{
  df::Navigator navigator;

  // Initialize.
  navigator.OnSize(200, 100);
  m2::PointD center = navigator.Screen().PixelRect().Center();
  navigator.SetFromRect(m2::AnyRectD(m2::RectD(0, 0, 8, 4)));
  TEST_EQUAL(navigator.Screen().ClipRect(), m2::RectD(0, 0, 8, 4), ());

  CheckNavigator(navigator);

  navigator.Scale(center, 3.0);
  CheckNavigator(navigator);

  navigator.Scale(center, 1 / 3.0);
  CheckNavigator(navigator);
}

UNIT_CLASS_TEST(VisualParamsFixture, Navigator_ToRotatedContainsTheWholeRect)
{
  // On a rotated map, the rect to fit keeps the map's heading and center, and contains the whole rect: its bounds in
  // the screen axes.
  df::Navigator navigator;
  navigator.OnSize(200, 100);
  double const kAngle = math::pi / 4;
  navigator.SetFromRect(m2::AnyRectD(m2::PointD(4, 2), ang::AngleD(kAngle), m2::RectD(-4, -2, 4, 2)));
  TEST(AlmostEqualAbs(navigator.Screen().GetAngle(), kAngle, 1e-9), (navigator.Screen().GetAngle()));

  m2::RectD const rect(0, 0, 8, 4);
  m2::AnyRectD const rotated = df::ToRotated(navigator, rect);
  TEST(AlmostEqualAbs(rotated.Angle().val(), kAngle, 1e-9), (rotated.Angle().val()));
  TEST(rotated.GlobalCenter().EqualDxDy(rect.Center(), 1e-9), (rotated.GlobalCenter()));

  // At 45 degrees each half-extent is (8 + 4) / 2 * cos(45).
  double const kHalfExtent = 6.0 * std::sqrt(0.5);
  m2::RectD const & local = rotated.GetLocalRect();
  TEST(AlmostEqualAbs(local.SizeX(), 2 * kHalfExtent, 1e-9), (local));
  TEST(AlmostEqualAbs(local.SizeY(), 2 * kHalfExtent, 1e-9), (local));
  for (auto const & corner : {rect.LeftBottom(), rect.LeftTop(), rect.RightTop(), rect.RightBottom()})
  {
    m2::PointD const pt = rotated.ConvertTo(corner) - local.Center();
    TEST_LESS_OR_EQUAL(std::fabs(pt.x), kHalfExtent + 1e-9, (corner));
    TEST_LESS_OR_EQUAL(std::fabs(pt.y), kHalfExtent + 1e-9, (corner));
  }
}
}  // namespace navigator_test
