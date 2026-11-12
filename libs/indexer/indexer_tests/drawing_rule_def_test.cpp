#include "testing/testing.hpp"

#include "indexer/drawing_rule_def.hpp"

namespace drawing_rule_def_test
{
using namespace drule;

// Representative drules priorities of BG-by-size areas (priorities_1_BG-by-size.prio.txt + kBasePriorityBgBySize),
// they may drift from the styles, the tests check the formula only.
int constexpr kCoastline = -1990, kIslet = -1980, kForest = -1940, kStadium = -1750;

UNIT_TEST(CalcAreaBySizeDepth_SimilarSizesFollowPriorities)
{
  // E.g. a forest multipolygon outer sharing a ring with an islet.
  m2::RectD const islet(0, 0, 1, 1);
  TEST_GREATER(CalcAreaBySizeDepth(islet, kForest), CalcAreaBySizeDepth(islet, kIslet), ());

  // A higher priority area stays on top while its bbox area is less than 2^(priorities diff / 500) bigger:
  // 2^(40/500) = 1.057 for forest over islet.
  TEST_GREATER(CalcAreaBySizeDepth({0, 0, 1.05, 1}, kForest), CalcAreaBySizeDepth(islet, kIslet), ());
  TEST_LESS(CalcAreaBySizeDepth({0, 0, 1.06, 1}, kForest), CalcAreaBySizeDepth(islet, kIslet), ());

  // 2^(240/500) = 1.395 for stadium over coastline.
  TEST_GREATER(CalcAreaBySizeDepth({0, 0, 1.39, 1}, kStadium), CalcAreaBySizeDepth(islet, kCoastline), ());
  TEST_LESS(CalcAreaBySizeDepth({0, 0, 1.40, 1}, kStadium), CalcAreaBySizeDepth(islet, kCoastline), ());
}

UNIT_TEST(CalcAreaBySizeDepth_Range)
{
  int constexpr kMinPriority = kBasePriorityBgBySize, kMaxPriority = kBasePriorityBgTop - 1;
  for (int priority : {kMinPriority, kMaxPriority})
  {
    // Bbox areas from zero to near the 2^13 maximum.
    for (double size : {0.0, 1.0E-6, 90.0})
    {
      double const depth = CalcAreaBySizeDepth({0, 0, size, size}, priority);
      TEST_GREATER_OR_EQUAL(depth, kBaseDepthBgBySize, (priority, size));
      TEST_LESS(depth, kBaseDepthBgTop, (priority, size));
    }
  }
}
}  // namespace drawing_rule_def_test
