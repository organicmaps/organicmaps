#include "testing/testing.hpp"

#include "storage/queued_country.hpp"
#include "storage/storage.hpp"

namespace storage
{
UNIT_TEST(QueuedCountry_AddOptions)
{
  Storage storage;
  TCountryId const countryId = storage.FindCountryIdByFile("USA_Georgia");
  QueuedCountry country(countryId, MapOptions::CarRouting);

  TEST_EQUAL(countryId, country.GetCountryId(), ());
  TEST_EQUAL(MapOptions::CarRouting, country.GetInitOptions(), ());
  TEST_EQUAL(MapOptions::CarRouting, country.GetCurrentFile(), ());

  country.AddOptions(MapOptions::Map);
  TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
  TEST_EQUAL(MapOptions::CarRouting, country.GetCurrentFile(), ());

  TEST(country.SwitchToNextFile(), ());
  TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
  TEST_EQUAL(MapOptions::Map, country.GetCurrentFile(), ());

  TEST(!country.SwitchToNextFile(), ());
  TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
  TEST_EQUAL(MapOptions::Nothing, country.GetCurrentFile(), ());
}

UNIT_TEST(QueuedCountry_RemoveOptions)
{
  Storage storage;
  TCountryId const countryId = storage.FindCountryIdByFile("USA_Georgia");

  {
    QueuedCountry country(countryId, MapOptions::MapWithCarRouting);
    TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::Map, country.GetCurrentFile(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetDownloadedFiles(), ());

    country.RemoveOptions(MapOptions::Map);
    TEST_EQUAL(MapOptions::CarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::CarRouting, country.GetCurrentFile(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetDownloadedFiles(), ());

    country.RemoveOptions(MapOptions::CarRouting);
    TEST_EQUAL(MapOptions::Nothing, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetCurrentFile(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetDownloadedFiles(), ());
  }

  {
    QueuedCountry country(countryId, MapOptions::MapWithCarRouting);
    TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::Map, country.GetCurrentFile(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetDownloadedFiles(), ());

    country.SwitchToNextFile();
    TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::CarRouting, country.GetCurrentFile(), ());
    TEST_EQUAL(MapOptions::Map, country.GetDownloadedFiles(), ());

    country.RemoveOptions(MapOptions::CarRouting);
    TEST_EQUAL(MapOptions::Map, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetCurrentFile(), ());
    TEST_EQUAL(MapOptions::Map, country.GetDownloadedFiles(), ());
  }

  {
    QueuedCountry country(countryId, MapOptions::MapWithCarRouting);
    TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::Map, country.GetCurrentFile(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetDownloadedFiles(), ());

    country.SwitchToNextFile();
    TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::CarRouting, country.GetCurrentFile(), ());
    TEST_EQUAL(MapOptions::Map, country.GetDownloadedFiles(), ());

    country.RemoveOptions(MapOptions::Map);
    TEST_EQUAL(MapOptions::CarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::CarRouting, country.GetCurrentFile(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetDownloadedFiles(), ());

    country.SwitchToNextFile();
    TEST_EQUAL(MapOptions::CarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetCurrentFile(), ());
    TEST_EQUAL(MapOptions::CarRouting, country.GetDownloadedFiles(), ());
  }
}
}  // namespace storage
