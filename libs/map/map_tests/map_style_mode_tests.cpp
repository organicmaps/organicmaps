#include "map/framework.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/map_style_reader.hpp"

#include "platform/settings.hpp"

#include "testing/testing.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace map_style_mode_tests
{
namespace
{
std::string_view constexpr kMapStyleKey = "MapStyleKeyV1";
std::string_view constexpr kMapStyleModeKey = "MapStyleMode";
std::string_view constexpr kLegacyCyclingEnabledKey = "CyclingEnabled";
std::string_view constexpr kLegacyOutdoorsEnabledKey = "OutdoorsEnabled";

std::array constexpr kStyleKeys = {kMapStyleKey, kMapStyleModeKey, kLegacyCyclingEnabledKey, kLegacyOutdoorsEnabledKey};

class MapStyleSettings
{
public:
  MapStyleSettings() : m_originalStyle(GetStyleReader().GetCurrentStyle())
  {
    for (size_t i = 0; i < kStyleKeys.size(); ++i)
    {
      std::string value;
      if (settings::Get(kStyleKeys[i], value))
        m_originalSettings[i] = std::move(value);
      settings::Delete(kStyleKeys[i]);
    }
  }

  ~MapStyleSettings()
  {
    for (size_t i = 0; i < kStyleKeys.size(); ++i)
      if (auto const & value = m_originalSettings[i])
        settings::Set(kStyleKeys[i], *value);
      else
        settings::Delete(kStyleKeys[i]);
    classificator::EnsureStyleLoaded(m_originalStyle);
    GetStyleReader().SetCurrentStyle(m_originalStyle);
  }

private:
  MapStyle const m_originalStyle;
  std::array<std::optional<std::string>, kStyleKeys.size()> m_originalSettings;
};
}  // namespace

UNIT_CLASS_TEST(MapStyleSettings, MigratedOutdoorsSurvivesVehicleNavigation)
{
  settings::Set(kMapStyleKey, std::string_view("MapStyleOutdoorsDark"));
  Framework framework(FrameworkParams(false /* enableDiffs */), false /* loadMaps */);
  TEST_EQUAL(framework.GetMapStyle(), MapStyleOutdoorsDark, ());

  framework.SetMapStyleForCurrentMode(true /* dark */, true /* vehicleStyle */);
  TEST_EQUAL(framework.GetMapStyle(), MapStyleVehicleDark, ());
  framework.SetMapStyleForCurrentMode(true /* dark */, false /* vehicleStyle */);
  TEST_EQUAL(framework.GetMapStyle(), MapStyleOutdoorsDark, ());

  std::string savedMode;
  TEST(settings::Get(kMapStyleModeKey, savedMode), ());
  TEST_EQUAL(savedMode, "Outdoors", ());
}

UNIT_CLASS_TEST(MapStyleSettings, LegacyModePrecedence)
{
  settings::Set(kMapStyleKey, std::string_view("MapStyleOutdoorsDark"));
  settings::Set(kLegacyOutdoorsEnabledKey, false);
  {
    Framework framework(FrameworkParams(false /* enableDiffs */), false /* loadMaps */);
    TEST_EQUAL(framework.GetMapStyle(), MapStyleDefaultDark, ());
  }

  settings::Delete(kMapStyleModeKey);
  settings::Set(kLegacyOutdoorsEnabledKey, true);
  settings::Set(kLegacyCyclingEnabledKey, true);
  {
    Framework framework(FrameworkParams(false /* enableDiffs */), false /* loadMaps */);
    TEST_EQUAL(framework.GetMapStyle(), MapStyleCyclingDark, ());
  }

  settings::Set(kMapStyleModeKey, std::string_view("Default"));
  Framework framework(FrameworkParams(false /* enableDiffs */), false /* loadMaps */);
  TEST_EQUAL(framework.GetMapStyle(), MapStyleDefaultDark, ());
}

UNIT_CLASS_TEST(MapStyleSettings, SelectedModeSurvivesNavigationAndRestart)
{
  {
    Framework framework(FrameworkParams(false /* enableDiffs */), false /* loadMaps */);
    framework.SetMapStyleModeEnabled(MapStyleMode::Outdoors, true);
    TEST_EQUAL(framework.GetMapStyle(), MapStyleOutdoorsLight, ());

    framework.SetMapStyleForCurrentMode(true /* dark */, true /* vehicleStyle */);
    framework.SetMapStyleModeEnabled(MapStyleMode::Cycling, true);
    framework.SetMapStyleModeEnabled(MapStyleMode::Outdoors, false);
    TEST_EQUAL(framework.GetMapStyle(), MapStyleVehicleDark, ());
    TEST(framework.GetMapStyleMode() == MapStyleMode::Cycling, ());
  }

  Framework framework(FrameworkParams(false /* enableDiffs */), false /* loadMaps */);
  TEST_EQUAL(framework.GetMapStyle(), MapStyleCyclingDark, ());
  framework.SetMapStyleModeEnabled(MapStyleMode::Cycling, false);
  TEST_EQUAL(framework.GetMapStyle(), MapStyleDefaultDark, ());
}

UNIT_CLASS_TEST(MapStyleSettings, DebugPreviewAndNavigationRequestAreIndependent)
{
  settings::Set(kMapStyleKey, std::string_view("MapStyleDefaultLight"));
  Framework framework(FrameworkParams(false /* enableDiffs */), false /* loadMaps */);
  search::SearchParams params;
  params.m_query = "?vdark";
  TEST(framework.ParseSearchQueryCommand(params), ());
  TEST_EQUAL(framework.GetMapStyle(), MapStyleVehicleDark, ());

  std::string savedStyle;
  TEST(settings::Get(kMapStyleKey, savedStyle), ());
  TEST_EQUAL(savedStyle, "MapStyleDefaultLight", ());
  framework.SetMapStyleModeEnabled(MapStyleMode::Cycling, true);
  TEST_EQUAL(framework.GetMapStyle(), MapStyleCyclingLight, ());

  TEST(framework.ParseSearchQueryCommand(params), ());
  framework.SetMapStyleForCurrentMode(true /* dark */, true /* vehicleStyle */);
  framework.SetMapStyleModeEnabled(MapStyleMode::Outdoors, true);
  TEST_EQUAL(framework.GetMapStyle(), MapStyleVehicleDark, ());
  framework.SetMapStyleForCurrentMode(true /* dark */, false /* vehicleStyle */);
  TEST_EQUAL(framework.GetMapStyle(), MapStyleOutdoorsDark, ());
}
}  // namespace map_style_mode_tests
