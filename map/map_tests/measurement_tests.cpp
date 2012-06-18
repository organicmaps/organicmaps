#include "../../testing/testing.hpp"

#include "../measurement_utils.hpp"


using namespace MeasurementUtils;

UNIT_TEST(Measurement_Smoke)
{
  typedef pair<double, char const *> PairT;

  PairT arr[] = {
    PairT(10.0, "10 m"),
    PairT(10.4, "10 m"),
    PairT(10.51, "11 m"),
    PairT(1000.0, "1.0 km"),
    PairT(1100.0, "1.1 km"),
    PairT(1140.0, "1.1 km"),
    PairT(1151.0, "1.2 km"),
    PairT(1500.0, "1.5 km"),
    PairT(1549.9, "1.5 km"),
    PairT(1551.0, "1.6 km"),
    PairT(10000.0, "10 km"),
    PairT(10400.0, "10 km"),
    PairT(10499.9, "10 km"),
    PairT(10501.0, "11 km")
  };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
  {
    string s;
    TEST(FormatDistance(arr[i].first, s), (arr[i]));
    TEST_EQUAL(s, arr[i].second, (arr[i]));
  }
}
