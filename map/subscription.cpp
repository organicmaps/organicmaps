#include "map/subscription.hpp"

#include "platform/platform.hpp"

#include "coding/sha1.hpp"

#include "base/assert.hpp"

namespace
{
std::string const kSubscriptionId = "SubscriptionId";

std::string GetSubscriptionId()
{
  return coding::SHA1::CalculateBase64ForString(GetPlatform().UniqueClientId());
}
}  // namespace

Subscription::Subscription()
{
  std::string id;
  if (GetPlatform().GetSecureStorage().Load(kSubscriptionId, id))
    m_subscriptionId = id;
  m_isActive = (GetSubscriptionId() == m_subscriptionId);
}

void Subscription::Register(SubscriptionListener * listener)
{
  CHECK(listener != nullptr, ());
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  m_listeners.emplace_back(listener);
  listener->OnSubscriptionChanged(IsActive());
}

bool Subscription::IsActive() const
{
  return m_isActive;
}

void Subscription::Validate()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  //TODO: check on server.
  bool isValid = false;

  m_isActive = isValid;
  if (isValid)
  {
    m_subscriptionId = GetSubscriptionId();
    GetPlatform().GetSecureStorage().Save(kSubscriptionId, m_subscriptionId);
  }
  else
  {
    GetPlatform().GetSecureStorage().Remove(kSubscriptionId);
  }

  for (auto & listener : m_listeners)
    listener->OnSubscriptionChanged(isValid);
}
