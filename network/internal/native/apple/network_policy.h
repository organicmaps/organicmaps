#pragma once

#include "network/network_policy.hpp"

@class NSDate;

namespace om::network::network_policy
{
enum Stage
{
  Ask,
  Always,
  Never,
  Today,
  NotToday
};

void SetStage(Stage state);
Stage GetStage();

bool CanUseNetwork();
bool IsActivePolicyDate();
NSDate * GetPolicyDate();
}  // namespace om::network::network_policy
