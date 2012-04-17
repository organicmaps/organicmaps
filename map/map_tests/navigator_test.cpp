#include "../../testing/testing.hpp"

#include "../navigator.hpp"

#include "../../geometry/screenbase.hpp"

#include "../../std/cmath.hpp"


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
  {
    // Initialize.
    navigator.OnSize(0, 0, 200, 100);
    navigator.SetFromRect(m2::AnyRectD(m2::RectD(0, 0, 8, 4)));
    TEST_EQUAL(navigator.Screen().ClipRect(), m2::RectD(0, 0, 8, 4), ());
  }
  navigator.StartScale(navigator.Screen().GtoP(m2::PointD(1, 1)),
                       navigator.Screen().GtoP(m2::PointD(7, 1)), 0);
  navigator.StopScale( navigator.Screen().GtoP(m2::PointD(1, 1)),
                       navigator.Screen().GtoP(m2::PointD(4, 1)), 1);
  TEST_EQUAL(navigator.Screen().ClipRect(), m2::RectD(-1, -1, 15, 7), ());
}

namespace
{
  void CheckNavigator(Navigator const & nav)
  {
    m2::PointD const pxP = nav.Screen().ClipRect().Center();
    m2::PointD const gP = nav.PtoG(pxP);
    m2::PointD const pxP2 = nav.GtoP(gP);
    TEST(m2::AlmostEqual(pxP, pxP2), (pxP, pxP2));
  }
}

UNIT_TEST(Navigator_G2P_P2G)
{
  Navigator nav;
  {
    // Initialize.
    nav.OnSize(0, 0, 200, 100);
    nav.SetFromRect(m2::AnyRectD(m2::RectD(0, 0, 8, 4)));
    TEST_EQUAL(nav.Screen().ClipRect(), m2::RectD(0, 0, 8, 4), ());
  }

  CheckNavigator(nav);

  nav.Scale(3.0);
  CheckNavigator(nav);

  nav.Move(math::pi / 4.0, 3.0);
  CheckNavigator(nav);
}
