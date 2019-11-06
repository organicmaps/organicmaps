#include "map/onboarding.hpp"

#include "platform/platform.hpp"

#include "base/url_helpers.hpp"

#include <array>
#include <cstdint>
#include <ctime>

#include "private.h"

namespace
{
auto constexpr kTipsCount = static_cast<uint8_t>(onboarding::Tip::Type::Count);

std::string const kBaseUrl = "/en/v2/mobilefront/";
std::string const kUTMParams = "&utm_source=maps.me&utm_medium=onboarding_button";
std::array<std::string, kTipsCount> const kTipsLinks = {
  kBaseUrl + "?utm_campaign=catalog_discovery" + kUTMParams,
  kBaseUrl + "search/?tag=181&utm_campaign=sample_discovery" + kUTMParams,
  ""};
}  // namespace

namespace onboarding
{
Tip GetTip()
{
  auto const tipIndex = std::time(nullptr) % kTipsCount;
  auto const url = kTipsLinks[tipIndex];

  return {static_cast<Tip::Type>(tipIndex), url.empty() ? url : base::url::Join(BOOKMARKS_CATALOG_FRONT_URL, url)};
}

bool CanShowTipButton() { return GetPlatform().IsConnected(); }
}  // namespace onboarding
