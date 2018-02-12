#pragma once

#include "platform/network_policy.hpp"

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

void CallPartnersApi(platform::PartnersApiFn fn, bool force = false);

void SetStage(Stage state);
Stage const GetStage();

bool CanUseNetwork();
}  // namespace network_policy
