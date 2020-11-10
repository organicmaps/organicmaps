#include "partners_api/ads/arsenal_ads.hpp"

namespace
{
std::initializer_list<std::string> const kSupportedLanguages = {"ru"};

std::initializer_list<storage::CountryId> const kSupportedCountriesMedic = {
  "Russia_Moscow",
  "Russia_Saint Petersburg"
};

std::initializer_list<storage::CountryId> const kSupportedCountriesFlat = {
  "Guatemala",
  "Honduras",
  "El Salvador",
  "Belize",
  "United States of America",
  "Greece",
  "Mexico",
  "Cuba",
  "Tunisia",
  "Jordan",
  "Brazil",
  "Tanzania",
  "Morocco",
  "Macedonia",
  "Haiti",
  "Egypt",
  "Maldives",
  "Kenya",
  "Dominican Republic",
  "Russia_Moscow Oblast_East",
  "Russia_Moscow Oblast_West",
  "Russia_Rostov Oblast",
  "Russia_Tatarstan",
  "Russia_Sverdlovsk Oblast_Ekaterinburg",
  "Russia_Voronezh Oblast",
  "Russia_Tver Oblast",
  "Russia_Tula Oblast",
  "Russia_Bashkortostan",
  "Russia_Smolensk Oblast",
  "Russia_Samara Oblast",
  "Russia_Nizhny Novgorod Oblast",
  "Russia_Chelyabinsk Oblast",
  "Russia_Vladimir Oblast",
  "Russia_Novosibirsk Oblast",
  "Russia_Pskov Oblast",
  "Russia_Kaluga Oblast",
  "Russia_Yaroslavl Oblast",
  "Russia_Lipetsk Oblast",
  "Russia_Novgorod Oblast",
  "Russia_Volgograd Oblast",
  "Russia_Perm Krai_South",
  "Russia_Saratov Oblast",
  "Russia_Krasnoyarsk Krai_South",
  "Russia_Ryazan Oblast",
  "Russia_Bryansk Oblast",
  "Russia_Ulyanovsk Oblast",
  "Russia_Kursk Oblast",
  "Russia_Orenburg Oblast",
  "Russia_Belgorod Oblast",
  "Russia_Vologda Oblast",
  "Russia_Tyumen Oblast",
  "Russia_Oryol Oblast",
  "Russia_Yugra_Surgut",
  "Russia_Murmansk Oblast",
  "Russia_Omsk Oblast",
  "Russia_Sverdlovsk Oblast_North",
  "Russia_Khabarovsk Krai",
  "Russia_Udmurt Republic",
  "Russia_Chuvashia",
  "Russia_Penza Oblast",
  "Russia_Tambov Oblast",
  "Russia_Arkhangelsk Oblast_Central",
  "Russia_Ivanovo Oblast",
  "Russia_Kirov Oblast",
  "Russia_Kostroma Oblast",
  "Russia_Republic of Kalmykia",
  "Russia_Tomsk Oblast",
  "Russia_Kurgan Oblast",
  "Russia_Republic of Mordovia",
  "Russia_Mari El",
  "Russia_Perm Krai_North",
  "Russia_Komi Republic",
  "Russia_Yugra_Khanty",
  "Russia_Yamalo-Nenets Autonomous Okrug",
  "Russia_Sakha Republic",
  "Russia_Amur Oblast",
  "Russia_Zabaykalsky Krai",
  "Russia_Khakassia",
  "Russia_Arkhangelsk Oblast_North",
  "Russia_Krasnoyarsk Krai_North",
  "Russia_Jewish Autonomous Oblast",
  "Russia_Magadan Oblast",
  "Russia_Nenets Autonomous Okrug",
  "Russia_Tuva"
};

std::initializer_list<storage::CountryId> const kSupportedCountriesCrimea = {"Crimea"};

std::initializer_list<storage::CountryId> const kSupportedCountriesRussia = {
  "Russia_Krasnodar Krai",
  "Russia_Krasnodar Krai_Adygeya",
  "Russia_Leningradskaya Oblast_Karelsky",
  "Russia_Leningradskaya Oblast_Southeast",
  "Russia_Kaliningrad Oblast",
  "Russia_Stavropol Krai",
  "Russia_Republic of Karelia_South",
  "Russia_Primorsky Krai",
  "Russia_Kabardino-Balkaria",
  "Russia_North Ossetia-Alania",
  "Russia_Irkutsk Oblast",
  "Russia_Kemerov Oblast",
  "Russia_Karachay-Cherkessia",
  "Russia_Republic of Dagestan",
  "Russia_Astrakhan Oblast",
  "Russia_Republic of Karelia_North",
  "Russia_Buryatia",
  "Russia_Altai Republic",
  "Russia_Sakhalin Oblast",
  "Russia_Ingushetia",
  "Russia_Chechen Republic",
  "Russia_Kamchatka Krai",
  "Russia_Chukotka Autonomous Okrug"
};

std::initializer_list<storage::CountryId> const kSupportedCountriesWorld = {
  "Ukraine",
  "Belarus",
  "Turkey",
  "Kazakhstan",
  "Abkhazia",
  "Croatia",
  "Bosnia and Herzegovina",
  "Slovenia",
  "Moldova",
  "Serbia",
  "United Arab Emirates",
  "Albania",
  "Montenegro",
  "Bahrain",
  "Bolivia",
  "Colombia",
  "Malta",
  "Peru",
  "South Ossetia",
  "South Africa"
};

std::initializer_list<storage::CountryId> const kSupportedUserPosCountriesRussia = {"Russian Federation"};
std::initializer_list<storage::CountryId> const kExcludedUserPosCountriesRussia = {"Russian Federation"};
}  // namespace

namespace ads
{
ArsenalMedic::ArsenalMedic(Delegate & delegate)
  : DownloadOnMapContainer(delegate)
{
  AppendSupportedUserLanguages(kSupportedLanguages);
  AppendSupportedCountries(kSupportedCountriesMedic);
}

std::string ArsenalMedic::GetBannerInternal() const
{
  return "https://arsenalins.ru/store/telemed.php?utm=maps_me_direct";
}

ArsenalFlat::ArsenalFlat(Delegate & delegate)
  : DownloadOnMapContainer(delegate)
{
  AppendSupportedUserLanguages(kSupportedLanguages);
  AppendSupportedCountries(kSupportedCountriesFlat);
}

std::string ArsenalFlat::GetBannerInternal() const
{
  return "https://arsenalins.ru/store/kvartiri.php?utm=maps_me_direct";
}

ArsenalInsuranceCrimea::ArsenalInsuranceCrimea(Delegate & delegate)
  : DownloadOnMapContainer(delegate)
{
  AppendSupportedUserLanguages(kSupportedLanguages);
  AppendSupportedCountries(kSupportedCountriesCrimea);
}

std::string ArsenalInsuranceCrimea::GetBannerInternal() const
{
  return "https://arsenalins.ru/store/crimea.php?utm=maps_me_direct";
}

ArsenalInsuranceRussia::ArsenalInsuranceRussia(Delegate & delegate)
  : DownloadOnMapContainer(delegate)
{
  AppendSupportedUserLanguages(kSupportedLanguages);
  AppendSupportedCountries(kSupportedCountriesRussia);
  AppendSupportedUserPosCountries(kSupportedUserPosCountriesRussia);
}

std::string ArsenalInsuranceRussia::GetBannerInternal() const
{
  return "https://arsenalins.ru/store/?utm=maps_me_direct&country=rus";
}

ArsenalInsuranceWorld::ArsenalInsuranceWorld(Delegate & delegate)
  : DownloadOnMapContainer(delegate)
{
  AppendSupportedUserLanguages(kSupportedLanguages);
  AppendSupportedCountries(kSupportedCountriesWorld);
  AppendExcludedUserPosCountries(kExcludedUserPosCountriesRussia);
}

std::string ArsenalInsuranceWorld::GetBannerInternal() const
{
  return "https://arsenalins.ru/store/?utm=maps_me_direct&country=not_rus";
}
}  // namespace ads
