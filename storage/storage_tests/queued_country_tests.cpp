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
  TEST_EQUAL(MapOptions::CarRouting, country.GetCurrentFileOptions(), ());

  country.AddOptions(MapOptions::Map);
  TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
  TEST_EQUAL(MapOptions::CarRouting, country.GetCurrentFileOptions(), ());

  TEST(country.SwitchToNextFile(), ());
  TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
  TEST_EQUAL(MapOptions::Map, country.GetCurrentFileOptions(), ());

  TEST(!country.SwitchToNextFile(), ());
  TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
  TEST_EQUAL(MapOptions::Nothing, country.GetCurrentFileOptions(), ());
}

UNIT_TEST(QueuedCountry_RemoveOptions)
{
  Storage storage;
  TCountryId const countryId = storage.FindCountryIdByFile("USA_Georgia");

  {
    QueuedCountry country(countryId, MapOptions::MapWithCarRouting);
    TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::Map, country.GetCurrentFileOptions(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetDownloadedFilesOptions(), ());

    country.RemoveOptions(MapOptions::Map);
    TEST_EQUAL(MapOptions::CarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::CarRouting, country.GetCurrentFileOptions(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetDownloadedFilesOptions(), ());

    country.RemoveOptions(MapOptions::CarRouting);
    TEST_EQUAL(MapOptions::Nothing, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetCurrentFileOptions(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetDownloadedFilesOptions(), ());
  }

  {
    QueuedCountry country(countryId, MapOptions::MapWithCarRouting);
    TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::Map, country.GetCurrentFileOptions(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetDownloadedFilesOptions(), ());

    country.SwitchToNextFile();
    TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::CarRouting, country.GetCurrentFileOptions(), ());
    TEST_EQUAL(MapOptions::Map, country.GetDownloadedFilesOptions(), ());

    country.RemoveOptions(MapOptions::CarRouting);
    TEST_EQUAL(MapOptions::Map, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetCurrentFileOptions(), ());
    TEST_EQUAL(MapOptions::Map, country.GetDownloadedFilesOptions(), ());
  }

  {
    QueuedCountry country(countryId, MapOptions::MapWithCarRouting);
    TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::Map, country.GetCurrentFileOptions(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetDownloadedFilesOptions(), ());

    country.SwitchToNextFile();
    TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::CarRouting, country.GetCurrentFileOptions(), ());
    TEST_EQUAL(MapOptions::Map, country.GetDownloadedFilesOptions(), ());

    country.RemoveOptions(MapOptions::Map);
    TEST_EQUAL(MapOptions::CarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::CarRouting, country.GetCurrentFileOptions(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetDownloadedFilesOptions(), ());

    country.SwitchToNextFile();
    TEST_EQUAL(MapOptions::CarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetCurrentFileOptions(), ());
    TEST_EQUAL(MapOptions::Nothing, country.GetCurrentFileOptions(), ());
    TEST_EQUAL(MapOptions::CarRouting, country.GetDownloadedFilesOptions(), ());
  }
}
}  // namespace storage
