#include "testing/testing.hpp"

#include "map/navigator.hpp"

#include "geometry/screenbase.hpp"

#include "std/cmath.hpp"
#include "std/bind.hpp"


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

UNIT_TEST(Navigator_Scale2Points)
{
  Navigator navigator;

  navigator.OnSize(0, 0, 200, 100);
  navigator.SetFromRect(m2::AnyRectD(m2::RectD(0, 0, 8, 4)));

  ScreenBase const & screen = navigator.Screen();
  TEST_EQUAL(screen.ClipRect(), m2::RectD(0, 0, 8, 4), ());

  navigator.StartScale(screen.GtoP(m2::PointD(1, 1)),
                       screen.GtoP(m2::PointD(7, 1)), 0);
  navigator.StopScale(screen.GtoP(m2::PointD(1, 1)),
                      screen.GtoP(m2::PointD(4, 1)), 1);
  TEST_EQUAL(screen.ClipRect(), m2::RectD(-1, -1, 15, 7), ());
}

namespace
{
  void CheckNavigator(Navigator const & nav)
  {
    typedef m2::PointD P;
    m2::RectD clipR = nav.Screen().ClipRect();

    P arr[] = { clipR.LeftTop(), clipR.RightTop(),
                clipR.RightBottom(), clipR.LeftBottom(),
                clipR.Center() };

    for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    {
      P const & pxP = arr[i];
      P const gP = nav.PtoG(pxP);
      P const pxP2 = nav.GtoP(gP);
      TEST(m2::AlmostEqualULPs(pxP, pxP2), (pxP, pxP2));
    }
  }
}

UNIT_TEST(Navigator_G2P_P2G)
{
  Navigator navigator;

  // Initialize.
  navigator.OnSize(0, 0, 200, 100);
  navigator.SetFromRect(m2::AnyRectD(m2::RectD(0, 0, 8, 4)));
  TEST_EQUAL(navigator.Screen().ClipRect(), m2::RectD(0, 0, 8, 4), ());

  CheckNavigator(navigator);

  navigator.Scale(3.0);
  CheckNavigator(navigator);

  navigator.Move(math::pi / 4.0, 3.0);
  CheckNavigator(navigator);

  navigator.Scale(1/3.0);
  CheckNavigator(navigator);
}
