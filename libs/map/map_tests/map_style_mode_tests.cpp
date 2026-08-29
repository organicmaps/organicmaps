#include "map/framework.hpp"

#include "indexer/map_style_reader.hpp"

#include "platform/settings.hpp"

#include "base/scope_guard.hpp"

#include "testing/testing.hpp"

#include <string>
#include <string_view>

namespace map_style_mode_tests
{
namespace
{
std::string_view constexpr kMapStyleKey = "MapStyleKeyV1";
std::string_view constexpr kMapStyleModeKey = "MapStyleMode";
std::string_view constexpr kLegacyCyclingEnabledKey = "CyclingEnabled";
std::string_view constexpr kLegacyOutdoorsEnabledKey = "OutdoorsEnabled";

template <typename T>
class SettingsSnapshot
{
public:
  explicit SettingsSnapshot(std::string_view key) : m_key(key), m_wasSet(settings::Get(key, m_value)) {}

  ~SettingsSnapshot()
  {
    if (m_wasSet)
      settings::Set(m_key, m_value);
    else
      settings::Delete(m_key);
  }

private:
  std::string m_key;
  T m_value{};
  bool m_wasSet = false;
};

class ScopedMapStyleSettings
{
public:
  ScopedMapStyleSettings()
  {
    settings::Delete(kMapStyleModeKey);
    settings::Delete(kLegacyCyclingEnabledKey);
    settings::Delete(kLegacyOutdoorsEnabledKey);
    settings::Delete(kMapStyleKey);
  }

private:
  SettingsSnapshot<std::string> m_mode{kMapStyleModeKey};
  SettingsSnapshot<bool> m_cycling{kLegacyCyclingEnabledKey};
  SettingsSnapshot<bool> m_outdoors{kLegacyOutdoorsEnabledKey};
  SettingsSnapshot<std::string> m_style{kMapStyleKey};
};
}  // namespace

UNIT_TEST(MapStyleMode_LoadLegacySettings)
{
  ScopedMapStyleSettings const settingsGuard;

  TEST(Framework::LoadMapStyleMode() == MapStyleMode::Default, ());

  settings::Set(kLegacyOutdoorsEnabledKey, true);
  TEST(Framework::LoadMapStyleMode() == MapStyleMode::Outdoors, ());

  settings::Set(kLegacyCyclingEnabledKey, true);
  TEST(Framework::LoadMapStyleMode() == MapStyleMode::Cycling, ());

  settings::Set(kMapStyleModeKey, std::string("Outdoors"));
  TEST(Framework::LoadMapStyleMode() == MapStyleMode::Outdoors, ());

  settings::Set(kMapStyleModeKey, std::string("Default"));
  TEST(Framework::LoadMapStyleMode() == MapStyleMode::Default, ());

  settings::Delete(kMapStyleModeKey);
  settings::Delete(kLegacyCyclingEnabledKey);
  settings::Delete(kLegacyOutdoorsEnabledKey);
  settings::Set(kMapStyleKey, MapStyleToString(MapStyleCyclingDark));
  TEST(Framework::LoadMapStyleMode() == MapStyleMode::Cycling, ());

  settings::Set(kMapStyleKey, MapStyleToString(MapStyleOutdoorsLight));
  TEST(Framework::LoadMapStyleMode() == MapStyleMode::Outdoors, ());

  settings::Set(kMapStyleKey, MapStyleToString(MapStyleVehicleDark));
  TEST(Framework::LoadMapStyleMode() == MapStyleMode::Default, ());
}

UNIT_TEST(MapStyleMode_RestoreSelectedModeAfterVehicleOverride)
{
  ScopedMapStyleSettings const settingsGuard;
  auto const originalStyle = GetStyleReader().GetCurrentStyle();
  SCOPE_GUARD(restoreStyle, [&]() { GetStyleReader().SetCurrentStyle(originalStyle); });

  settings::Set(kMapStyleModeKey, std::string("Cycling"));
  settings::Set(kMapStyleKey, MapStyleToString(MapStyleVehicleDark));

  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  TEST_EQUAL(framework.GetMapStyle(), MapStyleCyclingDark, ());
}

UNIT_TEST(MapStyleMode_MutualExclusionAndVehicleOverride)
{
  ScopedMapStyleSettings const settingsGuard;
  auto const originalStyle = GetStyleReader().GetCurrentStyle();
  SCOPE_GUARD(restoreStyle, [&]() { GetStyleReader().SetCurrentStyle(originalStyle); });

  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  TEST_EQUAL(framework.GetMapStyle(), MapStyleDefaultLight, ());

  framework.SetMapStyleModeEnabled(MapStyleMode::Outdoors, true);
  TEST(Framework::LoadMapStyleMode() == MapStyleMode::Outdoors, ());
  TEST_EQUAL(framework.GetMapStyle(), MapStyleOutdoorsLight, ());

  framework.SetMapStyleModeEnabled(MapStyleMode::Cycling, true);
  TEST(Framework::LoadMapStyleMode() == MapStyleMode::Cycling, ());
  TEST_EQUAL(framework.GetMapStyle(), MapStyleCyclingLight, ());

  // Disabling an inactive family must not change the selected one.
  framework.SetMapStyleModeEnabled(MapStyleMode::Outdoors, false);
  TEST(Framework::LoadMapStyleMode() == MapStyleMode::Cycling, ());
  TEST_EQUAL(framework.GetMapStyle(), MapStyleCyclingLight, ());

  framework.SetMapStyleModeEnabled(MapStyleMode::Cycling, false);
  TEST(Framework::LoadMapStyleMode() == MapStyleMode::Default, ());
  TEST_EQUAL(framework.GetMapStyle(), MapStyleDefaultLight, ());

  framework.SetMapStyleMode(MapStyleMode::Outdoors);
  framework.SetMapStyle(MapStyleVehicleDark);
  framework.SetMapStyleMode(MapStyleMode::Cycling);
  TEST(Framework::LoadMapStyleMode() == MapStyleMode::Cycling, ());
  TEST_EQUAL(framework.GetMapStyle(), MapStyleVehicleDark, ());

  // Platform theme synchronization restores the selected family after vehicle navigation.
  framework.SetMapStyle(GetMapStyleForMode(Framework::LoadMapStyleMode(), true /* dark */));
  TEST_EQUAL(framework.GetMapStyle(), MapStyleCyclingDark, ());
}
}  // namespace map_style_mode_tests
