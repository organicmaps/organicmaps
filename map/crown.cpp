#include "map/crown.hpp"

#include "map/purchase.hpp"

#include "metrics/eye.hpp"

#include "platform/preferred_languages.hpp"
#include "platform/platform.hpp"

#include <algorithm>
#include <array>
#include <string>

#include "private.h"

namespace
{
std::array<std::string, 5> const kSupportedLanguages = {"ru", "en", "fr", "de", "es"};
}  // namespace

namespace crown
{
bool NeedToShow(std::unique_ptr<Purchase> const & purchase)
{
  if (!purchase || purchase->IsSubscriptionActive(SubscriptionType::BookmarksAll) ||
      !GetPlatform().IsConnected())
  {
    return false;
  }

  auto const lang = languages::GetCurrentNorm();
  auto const supportedLanguageIt = std::find(kSupportedLanguages.cbegin(),
                                             kSupportedLanguages.cend(), lang);
  if (supportedLanguageIt == kSupportedLanguages.cend())
    return false;

  auto const eyeInfo = eye::Eye::Instance().GetInfo();
  // No need to show crown when it is clicked already.
  if (eyeInfo->m_crown.m_clickedTime.time_since_epoch().count() != 0)
    return false;

  // Show crown in some percent of devices.
  std::hash<std::string> h;
  auto const deviceHash = h(GetPlatform().UniqueClientId());
  LOG(LINFO, ("Crown device hash:", deviceHash));
  return deviceHash % 100 < CROWN_PERCENT_OF_DEVICES;
}
}  // namespace crown
