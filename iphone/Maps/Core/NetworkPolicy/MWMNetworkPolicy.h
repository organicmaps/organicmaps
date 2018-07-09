#pragma once

#include "platform/network_policy_ios.h"

namespace network_policy
{
void CallPartnersApi(platform::PartnersApiFn fn, bool force = false);
}  // namespace network_policy
