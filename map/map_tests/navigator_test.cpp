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

UNIT_TEST(NavigatorScale2Points)
{
  Navigator navigator;
  { // Initialize.
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
