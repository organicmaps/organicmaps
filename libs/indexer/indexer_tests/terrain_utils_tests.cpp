#include "testing/testing.hpp"

#include "indexer/terrain/terrain_utils.hpp"

namespace terrain_utils_tests
{
using measurement_utils::Units;

// The renderer relies on two invariants of the step tables (see the comments in
// terrain_utils.hpp): every traced step is a multiple of 10, or the isoline style class
// mapping (kAltClasses %1000/%500/%100/%50/%10 in RuleDrawer::DrawDynamicIsolines) fails
// to resolve a line rule and the isolines silently vanish; and every label step is a
// multiple of the traced step, or the labeled levels never coincide with the traced ones.
UNIT_TEST(TerrainIsolinesSteps)
{
  for (auto const units : {Units::Metric, Units::Imperial})
  {
    for (int zoom = 1; zoom <= 20; ++zoom)
    {
      int32_t const step = terrain::GetIsolinesStepForZoom(zoom, units);
      TEST_GREATER(step, 0, (static_cast<int>(units), zoom));
      TEST_EQUAL(step % 10, 0, (static_cast<int>(units), zoom));

      // Both branches of the altitude range condition.
      for (int32_t const range : {0, 100000})
      {
        int32_t const labelStep = terrain::GetIsolinesLabelStepForZoom(zoom, range, units);
        TEST_GREATER(labelStep, 0, (static_cast<int>(units), zoom, range));
        TEST_EQUAL(labelStep % step, 0, (static_cast<int>(units), zoom, range, step));
      }
    }
  }
}
}  // namespace terrain_utils_tests
