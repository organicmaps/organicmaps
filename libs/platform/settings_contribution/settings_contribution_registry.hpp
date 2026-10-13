#pragma once

#include "platform/settings_contribution/settings_contribution.hpp"

#include <vector>

namespace settings_contribution
{
// Process-wide list of SettingsContribution rows for host Settings UIs to append.
// Register() takes non-owning pointers with static storage duration (same contract as
// LocationProviderRegistry). Empty by default — hosts must hide empty sections.
class SettingsContributionRegistry
{
public:
  static SettingsContributionRegistry & Instance();

  void Register(SettingsContribution * contribution);

  std::vector<SettingsContribution *> const & Contributions() const { return m_contributions; }

private:
  std::vector<SettingsContribution *> m_contributions;
};
}  // namespace settings_contribution
