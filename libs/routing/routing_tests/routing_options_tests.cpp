#include "testing/testing.hpp"

#include "routing/router.hpp"
#include "routing/routing_options.hpp"

#include "platform/settings.hpp"

#include "base/scope_guard.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

using namespace routing;

namespace
{
using RoadType = RoutingOptions::RoadType;

class RoutingOptionsTests
{
private:
  RoutingOptionSetter m_car{0, VehicleType::Car};
  RoutingOptionSetter m_bicycle{0, VehicleType::Bicycle};
  RoutingOptionSetter m_pedestrian{0, VehicleType::Pedestrian};
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

  RoutingOptions::SaveToSettings(VehicleType::Car, options);
  RoutingOptions fromSettings = RoutingOptions::LoadFromSettings(VehicleType::Car);

  TEST_EQUAL(options.GetOptions(), fromSettings.GetOptions(), ());
}

UNIT_CLASS_TEST(RoutingOptionsTests, IndependentProfilesAndDefaults)
{
  for (auto const key :
       {"avoid_routing_options_car", "avoid_routing_options_bicycle", "avoid_routing_options_pedestrian"})
    settings::Delete(key);
  for (auto const vehicle : {VehicleType::Car, VehicleType::Bicycle, VehicleType::Pedestrian, VehicleType::Transit})
    TEST_EQUAL(RoutingOptions::LoadFromSettings(vehicle).GetOptions(), 0, (vehicle));

  settings::Set("avoid_routing_options_car", static_cast<uint32_t>(RoutingOptions::Toll | RoutingOptions::Dirty));
  TEST_EQUAL(RoutingOptions::LoadFromSettings(VehicleType::Car).GetOptions(),
             RoutingOptions::Toll | RoutingOptions::Dirty, ());
  TEST_EQUAL(RoutingOptions::LoadFromSettings(VehicleType::Bicycle).GetOptions(), 0, ());
  TEST_EQUAL(RoutingOptions::LoadFromSettings(VehicleType::Pedestrian).GetOptions(), 0, ());
  TEST_EQUAL(RoutingOptions::LoadFromSettings(VehicleType::Transit).GetOptions(), 0, ());

  RoutingOptions::SaveToSettings(VehicleType::Bicycle, CreateOptions({RoutingOptions::Ferry}));
  TEST(RoutingOptions::LoadFromSettings(VehicleType::Bicycle).Has(RoutingOptions::Ferry), ());
  TEST_EQUAL(RoutingOptions::LoadFromSettings(VehicleType::Pedestrian).GetOptions(), 0, ());
  RoutingOptions::SaveToSettings(VehicleType::Pedestrian, CreateOptions({RoutingOptions::Ferry}));
  RoutingOptions::SaveToSettings(VehicleType::Bicycle, {});
  TEST(RoutingOptions::LoadFromSettings(VehicleType::Pedestrian).Has(RoutingOptions::Ferry), ());
  TEST_EQUAL(RoutingOptions::LoadFromSettings(VehicleType::Bicycle).GetOptions(), 0, ());
  TEST_EQUAL(RoutingOptions::LoadFromSettings(VehicleType::Car).GetOptions(),
             RoutingOptions::Toll | RoutingOptions::Dirty, ());
}

UNIT_CLASS_TEST(RoutingOptionsTests, UnsupportedBitsAreSanitizedBeforeNarrowing)
{
  for (auto const vehicle : {VehicleType::Car, VehicleType::Bicycle, VehicleType::Pedestrian})
  {
    auto const key = vehicle == VehicleType::Car     ? "avoid_routing_options_car"
                   : vehicle == VehicleType::Bicycle ? "avoid_routing_options_bicycle"
                                                     : "avoid_routing_options_pedestrian";
    settings::Set(key, uint32_t{0xffffffff});
    TEST_EQUAL(RoutingOptions::LoadFromSettings(vehicle).GetOptions(), RoutingOptions::GetSupportedOptions(vehicle),
               (vehicle));
    RoutingOptions::SaveToSettings(vehicle, RoutingOptions(0xff));
    uint32_t stored = 0;
    TEST(settings::Get(key, stored), ());
    TEST_EQUAL(stored, RoutingOptions::GetSupportedOptions(vehicle), (vehicle));
    settings::Set(key, std::string("invalid"));
    TEST_EQUAL(RoutingOptions::LoadFromSettings(vehicle).GetOptions(), 0, (vehicle));
  }
  RoutingOptions::SaveToSettings(VehicleType::Transit, RoutingOptions(0xff));
  TEST_EQUAL(RoutingOptions::LoadFromSettings(VehicleType::Transit).GetOptions(), 0, ());
}

UNIT_CLASS_TEST(RoutingOptionsTests, SetterRestoresAbsentKeysAndRawValues)
{
  auto const key = "avoid_routing_options_bicycle";
  settings::Delete(key);
  {
    RoutingOptionSetter guard(RoutingOptions::Ferry, VehicleType::Bicycle);
    TEST(RoutingOptions::LoadFromSettings(VehicleType::Bicycle).Has(RoutingOptions::Ferry), ());
  }
  std::string value;
  TEST(!settings::Get(key, value), ());
  settings::Set(key, std::string("255"));
  {
    RoutingOptionSetter guard(0, VehicleType::Bicycle);
    TEST_EQUAL(RoutingOptions::LoadFromSettings(VehicleType::Bicycle).GetOptions(), 0, ());
  }
  TEST(settings::Get(key, value), ());
  TEST_EQUAL(value, "255", ());
}

UNIT_TEST(RoutingOptions_StableIdsAndRouterConversion)
{
  for (auto const road : {RoutingOptions::Usual, RoutingOptions::Toll, RoutingOptions::Motorway, RoutingOptions::Ferry,
                          RoutingOptions::Dirty, RoutingOptions::Steps})
  {
    auto const parsed = RoutingOptions::RoadFromId(static_cast<uint32_t>(road));
    TEST(parsed, (road));
    TEST_EQUAL(*parsed, road, ());
  }
  for (uint32_t const invalid : {0u, 3u, 6u, 33u, 64u, 256u, 0xffffffffu})
    TEST(!RoutingOptions::RoadFromId(invalid), (invalid));
  TEST(RoutingOptions::GetVehicleType(RouterType::Vehicle) == VehicleType::Car, ());
  TEST(RoutingOptions::GetVehicleType(RouterType::Bicycle) == VehicleType::Bicycle, ());
  TEST(RoutingOptions::GetVehicleType(RouterType::Pedestrian) == VehicleType::Pedestrian, ());
  TEST(RoutingOptions::GetVehicleType(RouterType::Transit) == VehicleType::Transit, ());
  TEST(!RoutingOptions::GetVehicleType(RouterType::Ruler), ());
  TEST_EQUAL(RoutingOptions::GetSupportedOptions(VehicleType::Car), 30, ());
  TEST_EQUAL(RoutingOptions::GetSupportedOptions(VehicleType::Bicycle), 8, ());
  TEST_EQUAL(RoutingOptions::GetSupportedOptions(VehicleType::Pedestrian), 8, ());
  TEST_EQUAL(RoutingOptions::GetSupportedOptions(VehicleType::Transit), 0, ());
}

UNIT_TEST(RouteOptimizationSettingDefaultsOffAndPersists)
{
  std::string_view constexpr kKey = "RouteOptimizationEnabled";
  std::string saved;
  bool const existed = settings::Get(kKey, saved);
  SCOPE_GUARD(restoreSetting, [&]
  {
    if (existed)
      settings::Set(kKey, saved);
    else
      settings::Delete(kKey);
  });

  settings::Delete(kKey);
  TEST(!RoutingOptions::LoadRouteOptimizationFromSettings(), ());
  for (bool const enabled : {true, false})
  {
    RoutingOptions::SaveRouteOptimizationToSettings(enabled);
    TEST_EQUAL(RoutingOptions::LoadRouteOptimizationFromSettings(), enabled, ());
  }
}
}  // namespace
