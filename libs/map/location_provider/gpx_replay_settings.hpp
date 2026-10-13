#pragma once

#ifdef DEBUG

#include "platform/settings_contribution/settings_contribution.hpp"

#include <string>
#include <vector>

namespace location_provider
{
// Settings → Debug → "Mock GPX file": opens a host file picker (extensions: gpx), stores path.
class GpxReplayFileContribution : public settings_contribution::SettingsContribution
{
public:
  static GpxReplayFileContribution & Instance();

  std::string GetId() const override { return "gpx_replay_file"; }
  std::string GetTitle() const override { return "Mock GPX file"; }
  std::string GetSectionId() const override { return "debug"; }
  std::string GetDetail() const override;
  std::vector<std::string> GetPickFileExtensions() const override { return {"gpx"}; }
  void OnFilePicked(std::string const & path) override;
};

// Settings → Debug → "Arm mock GPX" / "Stop mock GPX": click toggles arm from stored path.
class GpxReplayArmContribution : public settings_contribution::SettingsContribution
{
public:
  static GpxReplayArmContribution & Instance();

  std::string GetId() const override { return "gpx_replay_arm"; }
  std::string GetTitle() const override;
  std::string GetSectionId() const override { return "debug"; }
  std::string GetDetail() const override;
  void OnSelected() override;
};
}  // namespace location_provider

#endif  // DEBUG
