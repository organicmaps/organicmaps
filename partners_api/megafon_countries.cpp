#include "partners_api/megafon_countries.hpp"

#include <algorithm>

namespace ads
{
namespace
{
storage::CountriesVec const kCountries = {
    // All supported countries are removed until Megafon contract is extended or renewed.
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
