#pragma once

namespace om::network
{
class NetworkPolicy
{
public:
  explicit NetworkPolicy(bool const canUseNetwork) : m_canUse(canUseNetwork) {}

  bool CanUse() const { return m_canUse; }

private:
  bool m_canUse = false;
};

NetworkPolicy GetCurrentNetworkPolicy();
}  // namespace om::network
