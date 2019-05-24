#pragma once

#include "storage/storage.hpp"

#include <cstdint>
#include <string>

namespace promo
{
// Do not change the order.
enum class DownloaderPromoType : uint8_t
{
  NoPromo = 0,
  BookmarkCatalog = 1,
  Megafon = 2
};

struct DownloaderPromoBanner
{
  DownloaderPromoBanner() = default;
  DownloaderPromoBanner(DownloaderPromoType type, std::string const & url)
    : m_type(type)
    , m_url(url)
  {}

  DownloaderPromoType m_type = DownloaderPromoType::NoPromo;
  std::string m_url;
};

class DownloaderPromo
{
public:
  static DownloaderPromoBanner GetBanner(storage::Storage const & storage,
                                         std::string const & mwmId,
                                         std::string const & currentLocale,
                                         bool hasRemoveAdsSubscription);
};
}  // namespace promo
