#include "testing/testing.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"
#include "indexer/terrain/terrain_utils.hpp"

#include <initializer_list>
#include <utility>

namespace terrain_utils_tests
{
using measurement_utils::Units;

// The isolines drawing policy is resolved from the style (see terrain::IsolinesStyle),
// so its invariants are checked over the real shipped styles. The renderer relies on:
// the trace steps forming a nested ladder growing finer with zoom (so the levels of an
// upper zoom survive on a deeper one), every traced level resolving its line drule
// (or the isolines silently vanish), every labeled level being a traced one, and the
// sea depths mirroring the land altitudes.
UNIT_TEST(TerrainIsolinesStyle)
{
  for (auto const style : {MapStyleDefaultLight, MapStyleOutdoorsLight})
  {
    GetStyleReader().SetCurrentStyle(style);
    classificator::Load();

    for (auto const units : {Units::Metric, Units::Imperial})
    {
      int32_t prevStep = 0;
      bool seenVisible = false, seenZero = false;
      for (int zoom = 1; zoom <= scales::GetUpperStyleScale(); ++zoom)
      {
        terrain::IsolinesStyle const isolinesStyle(zoom, units);
        int32_t const step = isolinesStyle.GetStep();

        // The zero altitude line is a class of its own: once it appears, it stays, and
        // its label (if any) keeps the primary caption the renderer dereferences.
        if (isolinesStyle.GetLineRule(0) != nullptr)
          seenZero = true;
        else
          TEST(!seenZero, (static_cast<int>(style), static_cast<int>(units), zoom));
        if (auto const * zeroText = isolinesStyle.GetPathTextRule(0))
          TEST(zeroText->primary.has_value(), (zoom));

        if (step == 0)
        {
          // Once the isolines appear, they never disappear on the deeper zooms.
          TEST(!seenVisible, (static_cast<int>(style), static_cast<int>(units), zoom));
          continue;
        }
        seenVisible = true;

        TEST_EQUAL(step % 10, 0, (static_cast<int>(units), zoom, step));
        if (prevStep != 0)
        {
          TEST_LESS_OR_EQUAL(step, prevStep, (static_cast<int>(units), zoom));
          TEST_EQUAL(prevStep % step, 0, (static_cast<int>(units), zoom, prevStep, step));
        }
        prevStep = step;

        // k = 50 reaches the step_500 class at the finest steps (500 m / 1000 ft).
        for (int32_t const k : {1, 2, 3, 4, 5, 10, 25, 50, 100})
        {
          int32_t const altitude = k * step;
          // Every traced level draws, and the depths mirror the land.
          TEST(isolinesStyle.GetLineRule(altitude) != nullptr, (static_cast<int>(units), zoom, altitude));
          TEST_EQUAL(isolinesStyle.GetLineRule(altitude), isolinesStyle.GetLineRule(-altitude), (zoom, altitude));
          TEST_EQUAL(isolinesStyle.GetPathTextRule(altitude), isolinesStyle.GetPathTextRule(-altitude),
                     (zoom, altitude));

          // The label drules keep the primary caption (the renderer dereferences it).
          auto const * text = isolinesStyle.GetPathTextRule(altitude);
          if (text != nullptr)
            TEST(text->primary.has_value(), (zoom, altitude));
        }
      }
      // Both tested styles do show isolines (and the zero line) somewhere in the range.
      TEST(seenVisible, (static_cast<int>(style), static_cast<int>(units)));
      TEST(seenZero, (static_cast<int>(style), static_cast<int>(units)));
    }

    // The imperial ladder pairs 1:1 with the metric classes (see kClassDefs in
    // terrain_utils.cpp): the paired rungs resolve the very same drules per zoom.
    for (int zoom = 1; zoom <= scales::GetUpperStyleScale(); ++zoom)
    {
      terrain::IsolinesStyle const metric(zoom, Units::Metric);
      terrain::IsolinesStyle const imperial(zoom, Units::Imperial);
      for (auto const & [m, ft] : std::initializer_list<std::pair<int32_t, int32_t>>{
               {1000, 2000}, {500, 1000}, {100, 500}, {50, 100}, {10, 20}})
      {
        TEST_EQUAL(metric.GetLineRule(m), imperial.GetLineRule(ft), (zoom, m, ft));
        TEST_EQUAL(metric.GetPathTextRule(m), imperial.GetPathTextRule(ft), (zoom, m, ft));
      }
    }
  }
}
}  // namespace terrain_utils_tests
