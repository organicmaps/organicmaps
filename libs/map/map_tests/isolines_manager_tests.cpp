#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/isolines_manager.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/visual_params.hpp"

#include "terrain/terrain_utils.hpp"

#include "indexer/map_style.hpp"

#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "geometry/mercator.hpp"

#include "base/scope_guard.hpp"

#include <vector>

namespace isolines_manager_tests
{
using df::test_support::VisualParamsFixture;
using State = IsolinesManager::IsolinesState;

ScreenBase MakeScreen(int zoom, m2::PointD const & center = mercator::FromLatLon(43.7, 7.2))
{
  ScreenBase screen;
  screen.OnSize(0, 0, 1024, 1024);
  screen.SetFromRect(m2::AnyRectD(df::GetRectForDrawScale(zoom, center)));
  return screen;
}

UNIT_CLASS_TEST(VisualParamsFixture, IsolinesManager_States)
{
  IsolinesManager manager;
  std::vector<State> states;
  manager.SetStateListener([&](State state) { states.push_back(state); });
  TEST_EQUAL(manager.GetState(), State::Disabled, ());

  // A disabled layer only remembers the viewport.
  manager.UpdateViewport(MakeScreen(terrain::kMinIsolinesZoom + 2));
  TEST_EQUAL(manager.GetState(), State::Disabled, ());

  manager.SetEnabled(true);
  TEST(manager.IsVisible(), ());
  manager.SetHasTerrain(false);
  TEST_EQUAL(manager.GetState(), State::NoData, ());

  manager.SetHasTerrain(true);
  TEST_EQUAL(manager.GetState(), State::Enabled, ());

  // The low zooms keep the last state: no hint on each zoom bounce over the same place.
  manager.UpdateViewport(MakeScreen(terrain::kMinIsolinesZoom - 2));
  TEST(!manager.IsVisible(), ());
  TEST_EQUAL(manager.GetState(), State::Enabled, ());
  manager.UpdateViewport(MakeScreen(terrain::kMinIsolinesZoom));
  manager.SetHasTerrain(false);
  TEST_EQUAL(manager.GetState(), State::NoData, ());

  manager.SetEnabled(false);
  TEST(!manager.IsEnabled(), ());
  TEST_EQUAL(manager.GetState(), State::Disabled, ());
  TEST_EQUAL(states,
             (std::vector<State>{State::Enabled, State::NoData, State::Enabled, State::NoData, State::Disabled}), ());
}

class IsolinesTestFramework final : public Framework
{
public:
  IsolinesTestFramework() : Framework({}, false /* loadMaps */)
  {
    // Fake a cached hint for the empty (ocean) region set, like for an installed map without terrain.
    // Relies on the HasViewportTerrain cache: a real empty set never needs the hint.
    m_terrainRegistered = true;
    m_terrainHintMissing = true;
  }

  void SetViewport(ScreenBase const & screen) { OnViewportChanged(screen); }
};

UNIT_CLASS_TEST(VisualParamsFixture, IsolinesManager_NoHintInCarNavigation)
{
  WritableDirChanger const writableDirChanger("isolines_hint_tests",
                                              WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  IsolinesTestFramework framework;
  std::vector<State> states;
  framework.GetIsolinesManager().SetStateListener([&](State state) { states.push_back(state); });
  // The map style is process-wide: don't leak the vehicle one into the next tests on a failure.
  SCOPE_GUARD(restoreStyle, [&framework] { framework.SetMapStyle(kDefaultMapStyle); });

  // The vehicle styles draw no terrain: no hint.
  framework.SetMapStyle(MapStyleVehicleLight);
  framework.SetIsolinesEnabled(true);
  framework.SetViewport(MakeScreen(terrain::kMinIsolinesZoom + 2, mercator::FromLatLon(0.0, -140.0)));
  TEST_EQUAL(framework.GetIsolinesManager().GetState(), State::Enabled, ());

  // Leaving the vehicle style shows the hint for the same viewport.
  framework.SetMapStyle(kDefaultMapStyle);
  TEST_EQUAL(framework.GetIsolinesManager().GetState(), State::NoData, ());
  TEST_EQUAL(states, (std::vector<State>{State::Enabled, State::NoData}), ());
}
}  // namespace isolines_manager_tests
