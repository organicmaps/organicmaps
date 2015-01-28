#include "../../testing/testing.hpp"

#include "../route_track.hpp"

UNIT_TEST(clipArrowBodyAndGetArrowDirection)
{
  vector<m2::PointD> ptsTurn = {m2::PointD(4452766.0004956936, -8008660.158053305),
                                m2::PointD(4452767.909028396, -8008670.0188056063),
                                m2::PointD(4452768.5452059628, -8008681.4700018261),
                                m2::PointD(4452765.6824069088, -8008693.8754644003),
                                m2::PointD(4452759.3206312312, -8008705.6447494039),
                                m2::PointD(4452746.2789910901, -8008720.9130110294),
                                m2::PointD(4452746.2789910901, -8008720.9130110294),
                                m2::PointD(4452670.8919493081, -8008780.7137024049),
                                m2::PointD(4452631.7670288859, -8008811.5683144433),
                                m2::PointD(4452567.5130945379, -8008863.0986974351)};
  pair<m2::PointD, m2::PointD> arrowDirection;
  bool result = clipArrowBodyAndGetArrowDirection(ptsTurn, arrowDirection, 5, 13., 13., 19.);

  TEST(result, ());

  TEST(m2::AlmostEqual(arrowDirection.first, m2::PointD(4452740.7948958352, -8008725.2632638067)), ());
  TEST(m2::AlmostEqual(arrowDirection.second, m2::PointD(4452736.0942427581, -8008728.9920519013)), ());

  TEST_EQUAL(ptsTurn.size(), 4, ());
  if (ptsTurn.size() == 4)
  {
    TEST(m2::AlmostEqual(ptsTurn[0], m2::PointD(4452754.7223071428, -8008711.0281532137)), ());
    TEST(m2::AlmostEqual(ptsTurn[1], m2::PointD(4452746.2789910901, -8008720.9130110294)), ());
    TEST(m2::AlmostEqual(ptsTurn[2], m2::PointD(4452746.2789910901, -8008720.9130110294)), ());
    TEST(m2::AlmostEqual(ptsTurn[3], m2::PointD(4452736.0942427581, -8008728.9920519013)), ());
  }

}
