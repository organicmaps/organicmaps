#include "testing/testing.hpp"
#include "indexer/scales.hpp"


using namespace scales;

UNIT_TEST(ScaleLevel_Smoke)
{
  for (int level = 1; level < GetUpperScale(); ++level)
  {
    double const d = GetRationForLevel(level);
    int test = GetScaleLevel(d);
    TEST_EQUAL(level, test, ());

    m2::RectD const r = GetRectForLevel(level, m2::PointD(0.0, 0.0));
    test = GetScaleLevel(r);
    TEST_EQUAL(level, test, ());
  }
}
