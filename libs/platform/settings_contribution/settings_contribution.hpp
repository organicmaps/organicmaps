#pragma once

#include <string>
#include <vector>

namespace settings_contribution
{
// Declares one Settings row a plugin wants the host UI to show. Hosts map GetSectionId() to a
// UI section (e.g. "debug" → Debug). Non-owning registration via SettingsContributionRegistry.
//
// If GetPickFileExtensions() is non-empty, the host must present a file picker filtered to those
// extensions and call OnFilePicked() with a readable local path. Otherwise the host calls
// OnSelected() on tap.
class SettingsContribution
{
public:
  virtual ~SettingsContribution() = default;

  virtual std::string GetId() const = 0;
  virtual std::string GetTitle() const = 0;
  virtual std::string GetSectionId() const = 0;
  virtual std::string GetDetail() const { return {}; }

  // e.g. {"gpx"} — empty means click-only (OnSelected).
  virtual std::vector<std::string> GetPickFileExtensions() const { return {}; }

  virtual void OnSelected() {}
  virtual void OnFilePicked(std::string const & /* path */) {}
};
}  // namespace settings_contribution
