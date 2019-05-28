#pragma once

#include "storage/storage.hpp"

#include <cstdint>
#include <string>

namespace promo
{
class DownloaderPromo
{
public:
  // Do not change the order.
  enum class Type : uint8_t
  {
    NoPromo = 0,
    BookmarkCatalog = 1,
    Megafon = 2
  };

  struct Banner
  {
    Banner() = default;
    Banner(Type type, std::string const & url)
      : m_type(type)
      , m_url(url)
    {}

    Type m_type = Type::NoPromo;
    std::string m_url;
  };

  static Banner GetBanner(storage::Storage const & storage, std::string const & mwmId,
                          std::string const & currentLocale, bool hasRemoveAdsSubscription);
};
}  // namespace promo
