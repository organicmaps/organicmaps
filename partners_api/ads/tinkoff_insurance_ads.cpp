#include "partners_api/ads/tinkoff_insurance_ads.hpp"

namespace
{
std::initializer_list<std::string> const kSupportedLanguages = {"ru"};

std::initializer_list<storage::CountryId> const kSupportedCountries = {
  "France",
  "Spain",
  "Czech",
  "Crimea",
  "Netherlands",
  "Austria",
  "Greece",
  "Slovakia",
  "Estonia",
  "Hungary",
  "Lithuania",
  "Abkhazia",
  "Portugal",
  "Latvia",
  "Cyprus",
  "Romania",
  "Slovenia",
  "Indonesia",
  "Canada",
  "Vietnam",
  "United Arab Emirates",
  "Montenegro",
  "Croatia",
  "Bulgaria",
  "Denmark",
  "South Korea",
  "Brazil",
  "Mexico",
  "Philippines",
  "Israel",
  "South Africa",
  "Azerbaijan",
  "Serbia",
  "Australia",
  "Bosnia and Herzegovina",
  "Morocco",
  "Tunisia",
  "Sri Lanka",
  "Cuba",
  "Iran",
  "French Polynesia",
  "Nepal",
  "Ireland",
  "Malaysia",
  "Wallis and Futuna",
  "San Marino",
  "Monaco",
  "Moldova",
  "Singapore",
  "Dominican Republic",
  "Malta",
  "Jordan",
  "Palestine",
  "Jerusalem",
  "Maldives",
  "Andorra",
  "Tanzania",
  "Luxembourg",
  "Albania"
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
