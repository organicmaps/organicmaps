#include "partners_api/ads/mts_ads.hpp"

namespace
{
std::initializer_list<std::string> const kSupportedLanguages = {"ru"};

std::initializer_list<storage::CountryId> const kSupportedCountries = {
    "Italy",      "France",     "Germany",  "Spain",        "Ukraine",     "Turkey",
    "Belarus",    "Thailand",   "Portugal", "Cyprus",       "Romania",     "United Arab Emirates",
    "Kazakhstan", "Montenegro", "Croatia",  "Bulgaria",     "South Korea", "Armenia",
    "Israel",     "Tunisia",    "Egypt",    "Saudi Arabia", "Kyrgyzstan",  "Gibraltar"};

std::initializer_list<storage::CountryId> const kExcludedUserPosCountries = {"Russian Federation"};
}  // namespace

namespace ads
{
Mts::Mts(Delegate & delegate)
  : DownloadOnMapContainer(delegate)
{
  AppendSupportedUserLanguages(kSupportedLanguages);
  AppendSupportedCountries(kSupportedCountries);
  AppendExcludedUserPosCountries(kExcludedUserPosCountries);
}

std::string Mts::GetBannerInternal() const
{
  return "https://ad.adriver.ru/cgi-bin/click.cgi?"
         "sid=1&bt=103&ad=694606&pid=3030352&bid=6563398&bn=6563398&rnd=1435429146";
}
}  // namespace ads
