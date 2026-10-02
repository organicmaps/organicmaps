#include "platform/settings_contribution/settings_contribution_registry.hpp"

namespace settings_contribution
{
SettingsContributionRegistry & SettingsContributionRegistry::Instance()
{
  static SettingsContributionRegistry inst;
  return inst;
}

void SettingsContributionRegistry::Register(SettingsContribution * contribution)
{
  m_contributions.push_back(contribution);
}
}  // namespace settings_contribution
