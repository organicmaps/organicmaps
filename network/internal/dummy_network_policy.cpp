#include "network/network_policy.hpp"

namespace om::network
{
NetworkPolicy GetCurrentNetworkPolicy() { return NetworkPolicy{true}; }
}  // namespace om::network
