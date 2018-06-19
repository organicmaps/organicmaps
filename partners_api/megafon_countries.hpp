#pragma once

#include "storage/storage.hpp"

#include <string>

namespace ads
{
extern bool HasMegafonDownloaderBanner(storage::Storage const & storage, std::string const & mwmId);
extern std::string GetMegafonDownloaderBannerUrl();
}  // namespace ads
