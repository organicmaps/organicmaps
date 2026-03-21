#include "testing/testing.hpp"

#include "routing/routing_options.hpp"

#include <cstdint>
#include <vector>

namespace
{
using RoadType = routing::RoutingOptions::RoadType;

class RoutingOptionsTests
{
public:
  RoutingOptionsTests() { m_savedOptions = routing::RoutingOptions::LoadCarOptionsFromSettings(); }

  ~RoutingOptionsTests() { routing::RoutingOptions::SaveCarOptionsToSettings(m_savedOptions); }

private:
  routing::RoutingOptions m_savedOptions;
};

routing::RoutingOptions CreateOptions(std::vector<routing::RoutingOptions::Road> const & include)
{
  routing::RoutingOptions options;

  for (auto type : include)
    options.Add(type);

  return options;
}

void Checker(std::vector<routing::RoutingOptions::Road> const & include)
{
  routing::RoutingOptions options = CreateOptions(include);

  for (auto type : include)
    TEST(options.Has(type), ());

  auto max = static_cast<RoadType>(routing::RoutingOptions::Road::Max);
  for (uint8_t i = 1; i < max; i <<= 1)
  {
    bool hasInclude = false;
    auto type = static_cast<routing::RoutingOptions::Road>(i);
    for (auto has : include)
      hasInclude |= (type == has);

    if (!hasInclude)
      TEST(!options.Has(static_cast<routing::RoutingOptions::Road>(i)), ());
  }
}

UNIT_TEST(RoutingOptionTest)
{
  Checker({routing::RoutingOptions::Road::Toll, routing::RoutingOptions::Road::Motorway,
           routing::RoutingOptions::Road::Dirty});
  Checker({routing::RoutingOptions::Road::Toll, routing::RoutingOptions::Road::Dirty});

  Checker({routing::RoutingOptions::Road::Toll, routing::RoutingOptions::Road::Ferry,
           routing::RoutingOptions::Road::Dirty});

  Checker({routing::RoutingOptions::Road::Dirty});
  Checker({routing::RoutingOptions::Road::Toll});
  Checker({routing::RoutingOptions::Road::Dirty, routing::RoutingOptions::Road::Motorway});
  Checker({});
}

UNIT_CLASS_TEST(RoutingOptionsTests, GetSetTest)
{
  routing::RoutingOptions options =
      CreateOptions({routing::RoutingOptions::Road::Toll, routing::RoutingOptions::Road::Motorway,
                     routing::RoutingOptions::Road::Dirty});

  routing::RoutingOptions::SaveCarOptionsToSettings(options);
  routing::RoutingOptions fromSettings = routing::RoutingOptions::LoadCarOptionsFromSettings();

  TEST_EQUAL(options.GetOptions(), fromSettings.GetOptions(), ());
}
}  // namespace
