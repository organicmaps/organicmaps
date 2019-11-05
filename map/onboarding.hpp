#pragma once

#include <string>

namespace onboarding
{
struct Tip
{
  enum class Type
  {
    DiscoverCatalog,
    DownloadSamples,
    BuySubscription,

    Count
  };

  Tip(Type type, std::string const & url) : m_type(type), m_url(url) {}

  Type m_type;
  std::string m_url;
};

Tip GetTip();
bool CanShowTipButton();
}  // namespace onboarding
