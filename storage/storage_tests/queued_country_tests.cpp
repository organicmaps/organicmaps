#include "testing/testing.hpp"

#include "storage/queued_country.hpp"
#include "storage/storage.hpp"

namespace storage
{
UNIT_TEST(QueuedCountry_AddOptions)
{
  Storage storage;
  TIndex const index = storage.FindIndexByFile("USA_Georgia");
  QueuedCountry country(index, TMapOptions::CarRouting);

  TEST_EQUAL(index, country.GetIndex(), ());
  TEST_EQUAL(TMapOptions::CarRouting, country.GetInitOptions(), ());
  TEST_EQUAL(TMapOptions::CarRouting, country.GetCurrentFile(), ());

  country.AddOptions(TMapOptions::Map);
  TEST_EQUAL(TMapOptions::MapWithCarRouting, country.GetInitOptions(), ());
  TEST_EQUAL(TMapOptions::CarRouting, country.GetCurrentFile(), ());

  TEST(country.SwitchToNextFile(), ());
  TEST_EQUAL(TMapOptions::MapWithCarRouting, country.GetInitOptions(), ());
  TEST_EQUAL(TMapOptions::Map, country.GetCurrentFile(), ());

  TEST(!country.SwitchToNextFile(), ());
  TEST_EQUAL(TMapOptions::MapWithCarRouting, country.GetInitOptions(), ());
  TEST_EQUAL(TMapOptions::Nothing, country.GetCurrentFile(), ());
}

UNIT_TEST(QueuedCountry_RemoveOptions)
{
  Storage storage;
  TIndex const index = storage.FindIndexByFile("USA_Georgia");

  {
    QueuedCountry country(index, TMapOptions::MapWithCarRouting);
    TEST_EQUAL(TMapOptions::MapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::Map, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::Nothing, country.GetDownloadedFiles(), ());

    country.RemoveOptions(TMapOptions::Map);
    TEST_EQUAL(TMapOptions::CarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::CarRouting, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::Nothing, country.GetDownloadedFiles(), ());

    country.RemoveOptions(TMapOptions::CarRouting);
    TEST_EQUAL(TMapOptions::Nothing, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::Nothing, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::Nothing, country.GetDownloadedFiles(), ());
  }

  {
    QueuedCountry country(index, TMapOptions::MapWithCarRouting);
    TEST_EQUAL(TMapOptions::MapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::Map, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::Nothing, country.GetDownloadedFiles(), ());

    country.SwitchToNextFile();
    TEST_EQUAL(TMapOptions::MapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::CarRouting, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::Map, country.GetDownloadedFiles(), ());

    country.RemoveOptions(TMapOptions::CarRouting);
    TEST_EQUAL(TMapOptions::Map, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::Nothing, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::Map, country.GetDownloadedFiles(), ());
  }

  {
    QueuedCountry country(index, TMapOptions::MapWithCarRouting);
    TEST_EQUAL(TMapOptions::MapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::Map, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::Nothing, country.GetDownloadedFiles(), ());

    country.SwitchToNextFile();
    TEST_EQUAL(TMapOptions::MapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::CarRouting, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::Map, country.GetDownloadedFiles(), ());

    country.RemoveOptions(TMapOptions::Map);
    TEST_EQUAL(TMapOptions::CarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::CarRouting, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::Nothing, country.GetDownloadedFiles(), ());

    country.SwitchToNextFile();
    TEST_EQUAL(TMapOptions::CarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::Nothing, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::CarRouting, country.GetDownloadedFiles(), ());
  }
}

UNIT_TEST(QueuedCountry_Bits)
{
  Storage storage;
  TIndex const index = storage.FindIndexByFile("USA_Georgia");
  QueuedCountry country(index, TMapOptions::MapWithCarRouting);
  TEST_EQUAL(TMapOptions::Nothing, country.GetDownloadedFiles(), ());

  TEST(country.SwitchToNextFile(), ());
  TEST_EQUAL(TMapOptions::Map, country.GetDownloadedFiles(), ());

  TEST(!country.SwitchToNextFile(), ());
  TEST_EQUAL(TMapOptions::MapWithCarRouting, country.GetDownloadedFiles(), ());
}
}  // namespace storage
