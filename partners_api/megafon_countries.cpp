#include "partners_api/megafon_countries.hpp"

#include <algorithm>

namespace ads
{
namespace
{
storage::CountriesVec const kCountries = {
    "Abkhazia",
    "Albania",
    "Algeria",
    "Andorra",
    "Argentina",
    "Armenia",
    "Australia",
    "Austria",
    "Azerbaijan Region",
    "Bahrain",
    "Bangladesh",
    "Belarus",
    "Belgium",
    "Bolivia",
    "Bosnia and Herzegovina",
    "Brazil",
    "Bulgaria",
    "Cambodia",
    "Canada",
    "Chile",
    "Croatia",
    "Cyprus",
    "Czech Republic",
    "Denmark",
    "Dominican Republic",
    "Ecuador",
    "Egypt",
    "Estonia",
    "Faroe Islands",
    "Finland",
    "France",
    "Georgia Region",
    "Germany",
    "Gibraltar",
    "Greece",
    "Greenland",
    "Holland",
    "Hungary",
    "Iceland",
    "India",
    "Indonesia",
    "Ireland",
    "Israel Region",
    "Italy",
    "Japan",
    "Jordan",
    "Kazakhstan",
    "Kuwait",
    "Kyrgyzstan",
    "Latvia",
    "Liechtenstein",
    "Lithuania",
    "Luxembourg",
    "Macedonia",
    "Malaysia",
    "Malta",
    "Morocco",
    "Mexico",
    "Moldova",
    "Mongolia",
    "Montenegro",
    "Netherlands",
    "New Zealand",
    "Norway",
    "Oman",
    "Pakistan",
    "Panama",
    "People's Republic of China",
    "Peru",
    "Poland",
    "Portugal",
    "Qatar",
    "Romania",
    "San Marino",
    "Saudi Arabia",
    "Serbia",
    "Singapore",
    "Slovakia",
    "Slovenia",
    "South Africa",
    "South Korea",
    "South Ossetia",
    "Spain",
    "Sri Lanka",
    "Sweden",
    "Switzerland",
    "Taiwan",
    "Tajikistan",
    "Thailand",
    "Tunisia",
    "Turkey",
    "Ukraine",
    "United Arab Emirates",
    "United Kingdom",
    "United States of America",
    "Uruguay",
    "Uzbekistan",
    "Vietnam",
};

bool IsRussianLocale(std::string const & currentLocale)
{
  return currentLocale.find("ru") != std::string::npos;
}

bool ContainsCountry(storage::Storage const & storage, storage::CountriesVec const & countries,
                     storage::CountriesVec const & referenceCountries)
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

  storage::CountriesVec countries;
  storage.GetTopmostNodesFor(mwmId, countries);
  return ContainsCountry(storage, countries, kCountries);
}

bool HasMegafonCategoryBanner(storage::Storage const & storage,
                              storage::CountriesVec const & countries,
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
