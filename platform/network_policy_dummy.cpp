#include "platform/network_policy.hpp"

namespace platform
{
NetworkPolicy GetCurrentNetworkPolicy()
{
  return NetworkPolicy(true);
}
}  // namespace platform
