#include "testing/testing.hpp"
#include "helpers.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/drules_include.hpp"

namespace
{
double constexpr kMaxVisualScale = 4.;

double constexpr kMaxDashLength = 128 / kMaxVisualScale;
// Why 128? 7 bits are used to pack dash value (see stipple_pen_resource.cpp, StipplePenHandle::Init)
// Max value, which can be packed in 7 bits, is 128.
} // namespace

UNIT_TEST(Test_Dashes)
{
  styles::RunForEveryMapStyle([](MapStyle)
  {
    drule::rules().ForEachRule([](drule::BaseRule const * rule)
    {
      LineDefProto const * const line = rule->GetLine();
      if (nullptr == line || !line->has_dashdot())
        return;

      DashDotProto const & dd = line->dashdot();

      int const n = dd.dd_size();
      for (int i = 0; i < n; ++i)
      {
        double const value = dd.dd(i);
        TEST_GREATER_OR_EQUAL(value, 0.0, ());
        TEST_LESS_OR_EQUAL(value, kMaxDashLength, ());
      }
    });
  });
}
