#include "testing/testing.hpp"

#include "map/isolines_manager.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/terrain/terrain_utils.hpp"

#include "geometry/mercator.hpp"

#include <vector>

namespace isolines_manager_tests
{
using df::test_support::VisualParamsFixture;
using State = IsolinesManager::IsolinesState;

ScreenBase MakeScreen(int zoom)
{
  ScreenBase screen;
  screen.OnSize(0, 0, 1024, 1024);
  screen.SetFromRect(m2::AnyRectD(df::GetRectForDrawScale(zoom, mercator::FromLatLon(43.7, 7.2))));
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
}  // namespace isolines_manager_tests
