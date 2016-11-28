#include "std/function.hpp"

namespace networking_policy
{
using MWMNetworkingPolicyFn = function<void(BOOL solver)>;

void callPartnersApiWithNetworkingPolicy(MWMNetworkingPolicyFn const & fn);

enum class NetworkingPolicyState
{
  Always,
  Session,
  Never
};

void SetNetworkingPolicyState(NetworkingPolicyState state);
NetworkingPolicyState GetNetworkingPolicyState();
}  // namespace networking_policy
