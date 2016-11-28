#pragma once

class _jobject;
typedef _jobject * jobject;

namespace platform
{
/// Class that is used to allow or disallow remote calls.
class NetworkPolicy
{
  // Maker for android.
  friend NetworkPolicy ToNativeNetworkPolicy(jobject obj);
  // Maker for ios.
  // Dummy, real signature should be chosen by ios developer.
  friend NetworkPolicy MakeNetworkPolicyIos(bool canUseNetwork);

public:
  bool CanUse() const { return m_canUse; }

private:
  NetworkPolicy(bool const canUseNetwork) : m_canUse(canUseNetwork) {}

  bool m_canUse = false;
};
// Dummy, real signature, implementation and location should be chosen by ios developer.
inline NetworkPolicy MakeNetworkPolicyIos(bool canUseNetwork)
{
  return NetworkPolicy(canUseNetwork);
}
}  // namespace platform
