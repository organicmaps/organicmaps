#include "partners_api/ads/mastercard_sber_ads.hpp"

namespace
{
std::initializer_list<std::string> const kSupportedLanguages = {"ru"};

std::initializer_list<storage::CountryId> const kSupportedCountries = {
  "Russian Federation",
  "Italy",
  "Germany",
  "France",
  "Ukraine",
  "Poland",
  "Spain",
  "Belarus",
  "Czech Republic",
  "Turkey",
  "Netherlands",
  "Austria",
  "Greece",
  "Finland",
  "Romania",
  "Switzerland",
  "Lithuania",
  "Hungary",
  "Thailand",
  "Kazakhstan",
  "Norway",
  "Sweden",
  "Estonia",
  "Latvia",
  "Portugal",
  "Bulgaria",
  "Cyprus",
  "Croatia",
  "Armenia",
  "United Arab Emirates",
  "Montenegro",
  "Denmark",
  "South Korea",
  "Israel",
  "Egypt",
  "Kyrgyzstan",
  "Tunisia",
  "Saudi Arabia",
  "Malta",
  "Iceland",
  "Gibraltar",
  "United States of America",
  "United Kingdom",
  "Tanzania"
};
}  // namespace

namespace ads
{
MastercardSberbank::MastercardSberbank(Delegate & delegate)
  : DownloadOnMapContainer(delegate)
{
  AppendSupportedUserLanguages(kSupportedLanguages);
  AppendSupportedCountries(kSupportedCountries);
}

std::string MastercardSberbank::GetBannerInternal() const
{
  return "https://ad.doubleclick.net/ddm/trackclk/"
         "N144601.3594583MAPS.ME/B24472590.278434206;"
         "dc_trk_aid=472553463;dc_trk_cid=135308154;"
         "dc_lat=;dc_rdid=;tag_for_child_directed_treatment=;tfua";
}
}  // namespace ads
