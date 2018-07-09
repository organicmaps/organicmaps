#include "testing/testing.hpp"

#include "partners_api/megafon_countries.hpp"

UNIT_TEST(MegafonCountriesTest_ExistedCountry)
{
  storage::Storage storage;
  TEST(ads::HasMegafonDownloaderBanner(storage, "Germany_Baden-Wurttemberg_Regierungsbezirk Freiburg", "ru"), ());

  TEST(!ads::HasMegafonDownloaderBanner(storage, "Germany_Baden-Wurttemberg_Regierungsbezirk Freiburg", "en"), ());
}

UNIT_TEST(MegafonCountriesTest_NotExistedCountry)
{
  storage::Storage storage;
  TEST(!ads::HasMegafonDownloaderBanner(storage, "Russia_Altai Krai", "ru"), ());
}
