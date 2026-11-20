#include "testing/testing.hpp"

#include "routing/router.hpp"
#include "routing/routing_options.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

#include "platform/settings.hpp"

#include "base/scope_guard.hpp"

#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>
#include <thread>
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
  for (uint32_t i = 1; i < max; i <<= 1)
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

UNIT_TEST(RoutingOptions_StepsSubtypes)
{
  classificator::Load();
  auto const & classifier = RoutingOptionsClassifier::Instance(VehicleType::Bicycle);
  for (auto const & path :
       {std::vector<std::string>{"highway", "steps"}, {"highway", "steps", "bridge"}, {"highway", "steps", "tunnel"}})
    TEST(classifier.Get(classif().GetTypeByPath(path)) == RoutingOptions::Steps, (path));
}

UNIT_TEST(RoutingOptions_TrailGradesAndTruncatedSubtypes)
{
  classificator::Load();
  auto const & classifier = RoutingOptionsClassifier::Instance(VehicleType::Pedestrian);
  TEST(classifier.Get(classif().GetTypeByPath({"highway", "path", "difficult"})) == RoutingOptions::DifficultTrail, ());
  TEST(classifier.Get(classif().GetTypeByPath({"highway", "path", "expert"})) == RoutingOptions::ExpertTrail, ());
  TEST(classifier.Get(classif().GetTypeByPath({"highway", "motorway", "bridge"})) == RoutingOptions::Motorway, ());
}

UNIT_TEST(RoutingOptions_AndroidRoadIds)
{
  TEST(RoutingOptions::RoadFromId(1) == RoutingOptions::Usual, ());
  TEST(RoutingOptions::RoadFromId(2) == RoutingOptions::Toll, ());
  TEST(RoutingOptions::RoadFromId(4) == RoutingOptions::Motorway, ());
  TEST(RoutingOptions::RoadFromId(8) == RoutingOptions::Ferry, ());
  TEST(RoutingOptions::RoadFromId(16) == RoutingOptions::Dirty, ());
  TEST(RoutingOptions::RoadFromId(32) == RoutingOptions::Steps, ());

  // One switch covers both trail grades. ExpertTrail has no separate option ID.
  TEST(RoutingOptions::RoadFromId(64) == RoutingOptions::DifficultTrail, ());
  TEST(!RoutingOptions::RoadFromId(128), ());

  TEST(!RoutingOptions::RoadFromId(0), ());
  TEST(!RoutingOptions::RoadFromId(RoutingOptions::Toll | RoutingOptions::Ferry), ());
  TEST(!RoutingOptions::RoadFromId(0x100u | RoutingOptions::Toll), ());
}

UNIT_TEST(RoutingOptions_VehicleTypeForRouter)
{
  TEST(RoutingOptions::GetVehicleType(RouterType::Vehicle) == VehicleType::Car, ());
  TEST(RoutingOptions::GetVehicleType(RouterType::Bicycle) == VehicleType::Bicycle, ());
  TEST(RoutingOptions::GetVehicleType(RouterType::Pedestrian) == VehicleType::Pedestrian, ());
  TEST(RoutingOptions::GetVehicleType(RouterType::Transit) == VehicleType::Transit, ());
  TEST(!RoutingOptions::GetVehicleType(RouterType::Ruler), ());
}

UNIT_TEST(RoutingOptions_SupportedOptionsByVehicle)
{
  TEST_EQUAL(RoutingOptions::GetSupportedOptions(VehicleType::Car), 30, ());
  TEST_EQUAL(RoutingOptions::GetSupportedOptions(VehicleType::Bicycle),
             RoutingOptions::Dirty | RoutingOptions::Ferry | RoutingOptions::Steps, ());
  TEST_EQUAL(RoutingOptions::GetSupportedOptions(VehicleType::Pedestrian),
             RoutingOptions::Ferry | RoutingOptions::Steps | RoutingOptions::DifficultTrail, ());
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

UNIT_CLASS_TEST(RoutingOptionsTests, BicycleSurfaceChoicePreservesTheSavedHardAvoid)
{
  using enum BicycleSurfacePreference;
  std::string_view constexpr key = "BicycleSurfacePreference";
  auto const bicycle = VehicleType::Bicycle;
  TEST_EQUAL(RoutePlanningOptions::LoadBicycleSurfacePreference(), Any, ());
  settings::Set(key, 256u);
  TEST_EQUAL(RoutePlanningOptions::LoadBicycleSurfacePreference(), Any, ());
  RoutePlanningOptions::SaveBicycleSurfacePreference(PreferUnpaved);
  TEST_EQUAL(RoutePlanningOptions::LoadFromSettings(bicycle).m_bicycleSurfaceBias, BicycleSurfaceBias::PreferUnpaved,
             ());
  TEST_EQUAL(RoutePlanningOptions::LoadFromSettings(VehicleType::Car).m_bicycleSurfaceBias, BicycleSurfaceBias::Any,
             ());

  // A hard avoid from the place page takes priority over a saved preference.
  RoutingOptions const ferry(RoutingOptions::Ferry);
  RoutingOptions::SaveToSettings(bicycle, RoutingOptions(RoutingOptions::Dirty | RoutingOptions::Ferry));
  TEST_EQUAL(RoutePlanningOptions::LoadBicycleSurfacePreference(), AvoidUnpaved, ());
  auto const hardOptions = RoutePlanningOptions::LoadFromSettings(bicycle);
  TEST(hardOptions.m_avoid.Has(RoutingOptions::Dirty), ());
  TEST_EQUAL(hardOptions.m_bicycleSurfaceBias, BicycleSurfaceBias::Any, ());
  RoutingOptions::SaveToSettings(bicycle, ferry);
  TEST_EQUAL(RoutePlanningOptions::LoadBicycleSurfacePreference(), PreferUnpaved, ());

  RoutePlanningOptions::SaveBicycleSurfacePreference(AvoidUnpaved);
  TEST_EQUAL(RoutePlanningOptions::LoadBicycleSurfacePreference(), AvoidUnpaved, ());
  RoutingOptions::SaveToSettings(bicycle, ferry);
  TEST_EQUAL(RoutePlanningOptions::LoadBicycleSurfacePreference(), Any, ());

  RoutePlanningOptions::SaveBicycleSurfacePreference(AvoidUnpaved);
  RoutePlanningOptions::SaveBicycleSurfacePreference(PreferPaved);
  TEST_EQUAL(RoutePlanningOptions::LoadBicycleSurfacePreference(), PreferPaved, ());
  TEST_EQUAL(RoutingOptions::LoadFromSettings(bicycle), ferry, ());
}

UNIT_CLASS_TEST(RoutingOptionsTests, ConcurrentCyclingSurfaceChangesKeepTheRequestCoherent)
{
  using enum BicycleSurfacePreference;
  RoutePlanningOptions::SaveBicycleSurfacePreference(PreferPaved);
  std::atomic<bool> finished = false;
  std::jthread writer([&finished]
  {
    for (int i = 0; i < 500; ++i)
    {
      RoutePlanningOptions::SaveBicycleSurfacePreference(AvoidUnpaved);
      RoutePlanningOptions::SaveBicycleSurfacePreference(PreferPaved);
    }
    finished = true;
  });

  int invalidReads = 0;
  do
  {
    auto const preference = RoutePlanningOptions::LoadBicycleSurfacePreference();
    if (preference != PreferPaved && preference != AvoidUnpaved)
      ++invalidReads;
  }
  while (!finished);
  TEST_EQUAL(invalidReads, 0, ());
}

UNIT_TEST(TransitModeSettingsDefaultAndIgnoreUnsupportedBits)
{
  std::string_view constexpr key = "TransitAllowedModes";
  std::string saved;
  bool const existed = settings::Get(key, saved);
  SCOPE_GUARD(restore, [&]
  {
    if (existed)
      settings::Set(key, saved);
    else
      settings::Delete(key);
  });

  settings::Delete(key);
  TEST_EQUAL(RoutePlanningOptions::LoadTransitModes(), transit::kAllModes, ());
  auto const bus = static_cast<transit::ModeMask>(transit::Mode::Bus);
  RoutePlanningOptions::SaveTransitModes(bus);
  TEST_EQUAL(RoutePlanningOptions::LoadFromSettings(VehicleType::Transit).m_transitModes, bus, ());
  TEST_EQUAL(RoutePlanningOptions::LoadFromSettings(VehicleType::Pedestrian).m_transitModes, transit::kAllModes, ());
  settings::Set(key, 0x10000u | bus);
  TEST_EQUAL(RoutePlanningOptions::LoadTransitModes(), bus, ());
  settings::Set(key, 0x10000u);
  TEST_EQUAL(RoutePlanningOptions::LoadTransitModes(), transit::kAllModes, ());
}

UNIT_TEST(TransitRoutePreferencePersistsOnlyForTransit)
{
  std::string_view constexpr key = "TransitRoutePreference";
  std::string saved;
  bool const existed = settings::Get(key, saved);
  SCOPE_GUARD(restore, [&]
  {
    if (existed)
      settings::Set(key, saved);
    else
      settings::Delete(key);
  });
  using enum TransitPreference;
  settings::Delete(key);
  TEST_EQUAL(RoutePlanningOptions::LoadTransitPreference(), Recommended, ());
  for (auto const preference : {LessWalking, FewerTransfers, Recommended})
  {
    RoutePlanningOptions::SaveTransitPreference(static_cast<int64_t>(preference));
    TEST_EQUAL(RoutePlanningOptions::LoadFromSettings(VehicleType::Transit).m_transitPreference, preference, ());
    TEST_EQUAL(RoutePlanningOptions::LoadFromSettings(VehicleType::Pedestrian).m_transitPreference, Recommended, ());
  }
  settings::Set(key, 3u);
  TEST_EQUAL(RoutePlanningOptions::LoadTransitPreference(), Recommended, ());
}
}  // namespace
