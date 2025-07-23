#include "testing/testing.hpp"

#include "routing/routing_options.hpp"

#include <cstdint>
#include <vector>

using namespace routing;

namespace
{
using RoadType = RoutingOptions::RoadType;

class RoutingOptionsTests
{
public:
  RoutingOptionsTests() { m_savedOptions = RoutingOptions::LoadCarOptionsFromSettings(); }

  ~RoutingOptionsTests() { RoutingOptions::SaveCarOptionsToSettings(m_savedOptions); }

private:
  RoutingOptions m_savedOptions;
};

RoutingOptions CreateOptions(std::vector<RoutingOptions::Road> const & include)
{
  RoutingOptions options;

  for (auto type : include)
    options.Add(type);

  return options;
}

void Checker(std::vector<RoutingOptions::Road> const & include)
{
  RoutingOptions options = CreateOptions(include);

  for (auto type : include)
    TEST(options.Has(type), ());

  auto max = static_cast<RoadType>(RoutingOptions::Road::Max);
  for (uint8_t i = 1; i < max; i <<= 1)
  {
    bool hasInclude = false;
    auto type = static_cast<RoutingOptions::Road>(i);
    for (auto has : include)
      hasInclude |= (type == has);

    if (!hasInclude)
      TEST(!options.Has(static_cast<RoutingOptions::Road>(i)), ());
  }
}

UNIT_TEST(RoutingOptionTest)
{
  Checker({RoutingOptions::Road::Toll, RoutingOptions::Road::Motorway, RoutingOptions::Road::Dirty});
  Checker({RoutingOptions::Road::Toll, RoutingOptions::Road::Dirty});

  Checker({RoutingOptions::Road::Toll, RoutingOptions::Road::Ferry, RoutingOptions::Road::Dirty});

  Checker({RoutingOptions::Road::Dirty});
  Checker({RoutingOptions::Road::Toll});
  Checker({RoutingOptions::Road::Dirty, RoutingOptions::Road::Motorway});
  Checker({});
}

UNIT_CLASS_TEST(RoutingOptionsTests, GetSetTest)
{
  RoutingOptions options =
      CreateOptions({RoutingOptions::Road::Toll, RoutingOptions::Road::Motorway, RoutingOptions::Road::Dirty});

  RoutingOptions::SaveCarOptionsToSettings(options);
  RoutingOptions fromSettings = RoutingOptions::LoadCarOptionsFromSettings();

  TEST_EQUAL(options.GetOptions(), fromSettings.GetOptions(), ());
}
}  // namespace
