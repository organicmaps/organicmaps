#pragma once

#include "storage/storage.hpp"

#include <string>

namespace ads
{
extern bool HasMegafonDownloaderBanner(storage::Storage const & storage, std::string const & mwmId,
                                       std::string const & currentLocale);
extern std::string GetMegafonDownloaderBannerUrl();
}  // namespace ads
