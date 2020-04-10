#include "partners_api/ads/tinkoff_insurance_ads.hpp"

namespace
{
std::initializer_list<std::string> const kSupportedLanguages = {"ru"};

std::initializer_list<storage::CountryId> const kSupportedCountries = {
  "Abkhazia",
  "Albania",
  "Andorra",
  "Australia",
  "Austria",
  "Azerbaijan",
  "Bosnia and Herzegovina",
  "Brazil",
  "Bulgaria",
  "Canada",
  "Crimea",
  "Croatia",
  "Cuba",
  "Cyprus",
  "Czech Republic",
  "Denmark",
  "Dominican Republic",
  "Estonia",
  "France",
  "French Polynesia",
  "Greece",
  "Hungary",
  "Indonesia",
  "Iran",
  "Israel",
  "Jerusalem",
  "Jordan",
  "Latvia",
  "Lithuania",
  "Luxembourg",
  "Malaysia",
  "Maldives",
  "Malta",
  "Mexico",
  "Moldova",
  "Monaco",
  "Montenegro",
  "Morocco",
  "Nepal",
  "Netherlands",
  "Palestine",
  "Philippines",
  "Portugal",
  "Romania",
  "San Marino",
  "Serbia",
  "Singapore",
  "Slovakia",
  "Slovenia",
  "South Africa",
  "South Korea",
  "Spain",
  "Sri Lanka",
  "Tanzania",
  "Tunisia",
  "United Arab Emirates",
  "Vietnam",
  "Wallis and Futuna"
};

std::initializer_list<storage::CountryId> const kSupportedUserPosCountries = {"Russian Federation"};
}  // namespace

namespace ads
{
TinkoffInsurance::TinkoffInsurance(Delegate & delegate)
  : DownloadOnMapContainer(delegate)
{
  AppendSupportedUserLanguages(kSupportedLanguages);
  AppendSupportedCountries(kSupportedCountries);
  AppendSupportedUserPosCountries(kSupportedUserPosCountries);
}

std::string TinkoffInsurance::GetBannerInternal() const
{
  return "http://tinkoff.ru/insurance/travel/form/?"
         "utm_source=mapsme_vzr&utm_medium=dsp.fix&has_promo=1&promo=MAPSME10";
}
}  // namespace ads
