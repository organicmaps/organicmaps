#include "testing/testing.hpp"

#include "routing/routing_options.hpp"

#include <cstdint>
#include <vector>

namespace routing_options_tests
{
using namespace routing;

namespace
{
using RoadType = RoutingOptions::RoadType;
using Road = RoutingOptions::Road;
using enum RoutingOptions::Road;

class RoutingOptionsTests
{
public:
  RoutingOptionsTests() { m_savedOptions = RoutingOptions::LoadCarOptionsFromSettings(); }

  ~RoutingOptionsTests() { RoutingOptions::SaveCarOptionsToSettings(m_savedOptions); }

private:
  RoutingOptions m_savedOptions;
};

RoutingOptions CreateOptions(std::vector<Road> const & include)
{
  RoutingOptions options;

  for (auto type : include)
    options.Add(type);

  return options;
}

void Checker(std::vector<Road> const & include)
{
  RoutingOptions options = CreateOptions(include);

  for (auto type : include)
    TEST(options.Has(type), ());

  auto max = static_cast<RoadType>(Max);
  for (uint8_t i = 1; i < max; i <<= 1)
  {
    bool hasInclude = false;
    auto type = static_cast<Road>(i);
    for (auto has : include)
      hasInclude |= (type == has);

    if (!hasInclude)
      TEST(!options.Has(static_cast<Road>(i)), ());
  }
}

UNIT_TEST(RoutingOptionTest)
{
  Checker({Toll, Motorway, Dirty});
  Checker({Toll, Dirty});

  Checker({Toll, Ferry, Dirty});

  Checker({Dirty});
  Checker({Toll});
  Checker({Dirty, Motorway});
  Checker({});
}

UNIT_CLASS_TEST(RoutingOptionsTests, GetSetTest)
{
  RoutingOptions options = CreateOptions({Toll, Motorway, Dirty});

  RoutingOptions::SaveCarOptionsToSettings(options);
  RoutingOptions fromSettings = RoutingOptions::LoadCarOptionsFromSettings();

  TEST_EQUAL(options.GetOptions(), fromSettings.GetOptions(), ());
}
}  // namespace
}  // namespace routing_options_tests
