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
}  // namespace

bool HasMegafonDownloaderBanner(storage::Storage const & storage, std::string const & mwmId,
                                std::string const & currentLocale)
{
  if (currentLocale.find("ru") == std::string::npos)
    return false;

  storage::TCountriesVec countries;
  storage.GetTopmostNodesFor(mwmId, countries);
  for (auto const & c : countries)
  {
    if (std::find(kCountries.cbegin(), kCountries.cend(), c) != kCountries.cend())
      return true;
  }
  return false;
}

std::string GetMegafonDownloaderBannerUrl()
{
  return "https://localads.maps.me/redirects/megafon_downloader";
}
}  // namespace ads
