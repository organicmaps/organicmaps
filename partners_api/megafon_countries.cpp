#include "partners_api/megafon_countries.hpp"

#include <algorithm>

namespace ads
{
namespace
{
storage::TCountriesVec const kCountries = {
  "Armenia",
  "Austria",
  "Belarus",
  "Bulgaria",
  "Croatia",
  "Cyprus",
  "Czech",
  "Czech Republic",
  "Denmark",
  "Egypt",
  "Estonia",
  "Finland",
  "France",
  "Germany",
  "Gibraltar",
  "Greece",
  "Hungary",
  "Iceland",
  "Israel",
  "Italy",
  "Kazakhstan",
  "Kyrgyzstan",
  "Latvia",
  "Lithuania",
  "Malta",
  "Montenegro",
  "Netherlands",
  "Norway",
  "Poland",
  "Portugal",
  "Romania",
  "Saudi Arabia",
  "South Korea",
  "Spain",
  "Sweden",
  "Switzerland",
  "Thailand",
  "Tunisia",
  "Turkey",
  "Ukraine",
  "United Arab Emirates",
};

bool IsRussianLocale(std::string const & currentLocale)
{
  return currentLocale.find("ru") != std::string::npos;
}

bool ContainsCountry(storage::Storage const & storage, storage::TCountriesVec const & countries,
                     storage::TCountriesVec const & referenceCountries)
{
  for (auto const & c : countries)
  {
    if (std::find(referenceCountries.cbegin(), referenceCountries.cend(), c) !=
        referenceCountries.cend())
    {
      return true;
    }
  }
  return false;
}
}  // namespace

bool HasMegafonDownloaderBanner(storage::Storage const & storage, std::string const & mwmId,
                                std::string const & currentLocale)
{
  if (!IsRussianLocale(currentLocale))
    return false;

  storage::TCountriesVec countries;
  storage.GetTopmostNodesFor(mwmId, countries);
  return ContainsCountry(storage, countries, kCountries);
}

bool HasMegafonCategoryBanner(storage::Storage const & storage,
                              storage::TCountriesVec const & countries,
                              std::string const & currentLocale)
{
  if (!IsRussianLocale(currentLocale))
    return false;

  return ContainsCountry(storage, countries, kCountries);
}

std::string GetMegafonDownloaderBannerUrl()
{
  return "https://localads.maps.me/redirects/megafon_downloader";
}

std::string GetMegafonCategoryBannerUrl()
{
  return "https://localads.maps.me/redirects/megafon_search_category";
}
}  // namespace ads
