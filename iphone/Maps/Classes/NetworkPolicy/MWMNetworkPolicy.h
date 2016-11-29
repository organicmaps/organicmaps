#pragma once

#include "platform/network_policy.hpp"

namespace network_policy
{
void CallPartnersApi(platform::PartnersApiFn fn, bool force = false);

void SetStage(platform::NetworkPolicy::Stage state);
platform::NetworkPolicy::Stage GetStage();
}  // namespace network_policy
