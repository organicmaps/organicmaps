#include "helpers.hpp"
#include "testing/testing.hpp"

#include "drape/stipple_pen_resource.hpp"
#include "drape_frontend/visual_params.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/drules_include.hpp"

UNIT_TEST(Test_Dashes)
{
  styles::RunForEveryMapStyle([](MapStyle)
  {
    drule::rules().ForEachRule([](drule::BaseRule const * rule)
    {
      LineRuleProto const * const line = rule->GetLine();
      if (nullptr == line || !line->has_dashdot())
        return;

      DashDotProto const & dd = line->dashdot();

      int const n = dd.dd_size();
      if (n > 0)
      {
        TEST_GREATER_OR_EQUAL(n, 2, ());
        TEST_LESS_OR_EQUAL(n, 4, ());
        for (int i = 0; i < n; ++i)
        {
          double const value = dd.dd(i);
          TEST_GREATER_OR_EQUAL(value, 0.0, ());
        }

        double const patternLength = (dd.dd(0) + dd.dd(1)) * df::kMaxVisualScale;
        TEST_LESS_OR_EQUAL(patternLength, dp::kMaxStipplePenLength, (dd.dd(0), dd.dd(1)));
      }
    });
  });
}
