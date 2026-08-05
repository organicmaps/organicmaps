#include "testing/testing.hpp"

#include "routing/route_speed_settings.hpp"
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

class RouteSpeedSettingsTests
{
public:
  RouteSpeedSettingsTests()
    : m_savedPedestrian(LoadRouteSpeedSettings(VehicleType::Pedestrian))
    , m_savedBicycle(LoadRouteSpeedSettings(VehicleType::Bicycle))
  {}

  ~RouteSpeedSettingsTests()
  {
    SaveRouteSpeedSettings(VehicleType::Pedestrian, m_savedPedestrian);
    SaveRouteSpeedSettings(VehicleType::Bicycle, m_savedBicycle);
  }

private:
  RouteSpeedSettings const m_savedPedestrian;
  RouteSpeedSettings const m_savedBicycle;
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

UNIT_CLASS_TEST(RouteSpeedSettingsTests, SavesEachVehicleSeparately)
{
  SaveRouteSpeedSettings(VehicleType::Pedestrian, {6.5 /* km/h */});
  SaveRouteSpeedSettings(VehicleType::Bicycle, {28.0 /* km/h */, 8 /* m/s */, 225 /* from south-west */});

  auto const pedestrian = LoadRouteSpeedSettings(VehicleType::Pedestrian);
  TEST_ALMOST_EQUAL_ABS(pedestrian.m_cruisingSpeedKMpH, 6.5, 1e-9, ());
  TEST_EQUAL(pedestrian.m_windSpeedMpS, 0, ("A pedestrian has no wind setting"));

  auto const bicycle = LoadRouteSpeedSettings(VehicleType::Bicycle);
  TEST_ALMOST_EQUAL_ABS(bicycle.m_cruisingSpeedKMpH, 28.0, 1e-9, ());
  TEST_EQUAL(bicycle.m_windSpeedMpS, 8, ());
  TEST_EQUAL(bicycle.m_windDirectionDegrees, 225, ());
}

UNIT_CLASS_TEST(RouteSpeedSettingsTests, ClampsOutOfRangeValues)
{
  auto const range = GetCruisingSpeedRange(VehicleType::Bicycle);
  SaveRouteSpeedSettings(VehicleType::Bicycle, {range.m_max + 100.0, kMaxWindSpeedMpS + 10, -45});

  auto const bicycle = LoadRouteSpeedSettings(VehicleType::Bicycle);
  TEST_ALMOST_EQUAL_ABS(bicycle.m_cruisingSpeedKMpH, range.m_max, 1e-9, ());
  TEST_EQUAL(bicycle.m_windSpeedMpS, kMaxWindSpeedMpS, ());
  TEST_EQUAL(bicycle.m_windDirectionDegrees, 0, ());
}

UNIT_CLASS_TEST(RouteSpeedSettingsTests, IgnoresUnsupportedVehicles)
{
  TEST(!IsRouteSpeedSupported(VehicleType::Car), ());
  TEST(!IsWindSupported(VehicleType::Pedestrian), ());

  SaveRouteSpeedSettings(VehicleType::Car, {30.0 /* km/h */});
  TEST_ALMOST_EQUAL_ABS(LoadRouteSpeedSettings(VehicleType::Car).m_cruisingSpeedKMpH, 0.0, 1e-9, ());
}
}  // namespace
