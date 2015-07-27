#include "testing/testing.hpp"

#include "storage/queued_country.hpp"
#include "storage/storage.hpp"

namespace storage
{
UNIT_TEST(QueuedCountry_AddOptions)
{
  Storage storage;
  TIndex const index = storage.FindIndexByFile("USA_Georgia");
  QueuedCountry country(index, MapOptions::CarRouting);

  TEST_EQUAL(index, country.GetIndex(), ());
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
  TIndex const index = storage.FindIndexByFile("USA_Georgia");

  {
    QueuedCountry country(index, MapOptions::MapWithCarRouting);
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
    QueuedCountry country(index, MapOptions::MapWithCarRouting);
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
    QueuedCountry country(index, MapOptions::MapWithCarRouting);
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

UNIT_TEST(QueuedCountry_Bits)
{
  Storage storage;
  TIndex const index = storage.FindIndexByFile("USA_Georgia");
  QueuedCountry country(index, MapOptions::MapWithCarRouting);
  TEST_EQUAL(MapOptions::Nothing, country.GetDownloadedFiles(), ());

  TEST(country.SwitchToNextFile(), ());
  TEST_EQUAL(MapOptions::Map, country.GetDownloadedFiles(), ());

  TEST(!country.SwitchToNextFile(), ());
  TEST_EQUAL(MapOptions::MapWithCarRouting, country.GetDownloadedFiles(), ());
}
}  // namespace storage
