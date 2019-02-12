#pragma once

#include "storage/storage.hpp"

#include <string>

namespace ads
{
extern bool HasMegafonDownloaderBanner(storage::Storage const & storage, std::string const & mwmId,
                                       std::string const & currentLocale);
extern bool HasMegafonCategoryBanner(storage::Storage const & storage,
                                     storage::CountriesVec const & countries,
                                     std::string const & currentLocale);
extern std::string GetMegafonDownloaderBannerUrl();
extern std::string GetMegafonCategoryBannerUrl();
}  // namespace ads
