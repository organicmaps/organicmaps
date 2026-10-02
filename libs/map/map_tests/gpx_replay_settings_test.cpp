#ifdef DEBUG

#include "testing/testing.hpp"

#include "map/location_provider/gpx_replay_provider.hpp"
#include "map/location_provider/gpx_replay_settings.hpp"
#include "map/location_provider/gpx_replay_fusion_strategy.hpp"

#include "platform/platform.hpp"
#include "platform/settings_contribution/settings_contribution_registry.hpp"

namespace gpx_replay_settings_tests
{
namespace
{
std::string TestFixturePath(std::string const & name)
{
  return GetPlatform().TestsDataPathForFile("test_data/gpx/" + name);
}
}  // namespace

UNIT_TEST(SettingsContributionRegistry_DebugBuildRegistersGpxRows)
{
  location_provider::ForceLinkGpxReplayPlugin();
  auto const & contributions = settings_contribution::SettingsContributionRegistry::Instance().Contributions();
  TEST_EQUAL(contributions.size(), 2, ());
  TEST_EQUAL(contributions[0]->GetId(), "gpx_replay_file", ());
  TEST_EQUAL(contributions[1]->GetId(), "gpx_replay_arm", ());
  TEST(!contributions[0]->GetPickFileExtensions().empty(), ());
  TEST(contributions[1]->GetPickFileExtensions().empty(), ());
}

UNIT_TEST(GpxReplaySettings_FilePickThenArmThenStop)
{
  auto & provider = location_provider::GpxReplayProvider::Instance();
  provider.Disarm();
  provider.SetGpxPath({});

  std::string const path = TestFixturePath("at_springer_mountain.gpx");
  location_provider::GpxReplayFileContribution::Instance().OnFilePicked(path);
  TEST_EQUAL(provider.GetGpxPath(), path, ());
  TEST(!provider.IsArmed(), ());

  location_provider::GpxReplayArmContribution::Instance().OnSelected();
  TEST(provider.IsArmed(), ());

  location_provider::GpxReplayArmContribution::Instance().OnSelected();
  TEST(!provider.IsArmed(), ());

  provider.Disarm();
  provider.SetGpxPath({});
}

UNIT_TEST(GpxReplaySettings_ArmWithoutFileDoesNotArm)
{
  auto & provider = location_provider::GpxReplayProvider::Instance();
  provider.Disarm();
  provider.SetGpxPath({});

  location_provider::GpxReplayArmContribution::Instance().OnSelected();
  TEST(!provider.IsArmed(), ());
}
}  // namespace gpx_replay_settings_tests

#endif  // DEBUG
