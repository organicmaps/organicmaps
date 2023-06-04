#pragma once

#include "platform/network_policy.hpp"

@class NSDate;

namespace network_policy
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
NSDate* GetPolicyDate();
}  // namespace network_policy
