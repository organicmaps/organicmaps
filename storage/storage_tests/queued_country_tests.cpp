#include "testing/testing.hpp"

#include "storage/queued_country.hpp"
#include "storage/storage.hpp"

namespace storage
{
UNIT_TEST(QueuedCountry_AddOptions)
{
  Storage storage;
  TIndex const index = storage.FindIndexByFile("USA_Georgia");
  QueuedCountry country(index, TMapOptions::ECarRouting);

  TEST_EQUAL(index, country.GetIndex(), ());
  TEST_EQUAL(TMapOptions::ECarRouting, country.GetInitOptions(), ());
  TEST_EQUAL(TMapOptions::ECarRouting, country.GetCurrentFile(), ());

  country.AddOptions(TMapOptions::EMap);
  TEST_EQUAL(TMapOptions::EMapWithCarRouting, country.GetInitOptions(), ());
  TEST_EQUAL(TMapOptions::ECarRouting, country.GetCurrentFile(), ());

  TEST(country.SwitchToNextFile(), ());
  TEST_EQUAL(TMapOptions::EMapWithCarRouting, country.GetInitOptions(), ());
  TEST_EQUAL(TMapOptions::EMap, country.GetCurrentFile(), ());

  TEST(!country.SwitchToNextFile(), ());
  TEST_EQUAL(TMapOptions::EMapWithCarRouting, country.GetInitOptions(), ());
  TEST_EQUAL(TMapOptions::ENothing, country.GetCurrentFile(), ());
}

UNIT_TEST(QueuedCountry_RemoveOptions)
{
  Storage storage;
  TIndex const index = storage.FindIndexByFile("USA_Georgia");

  {
    QueuedCountry country(index, TMapOptions::EMapWithCarRouting);
    TEST_EQUAL(TMapOptions::EMapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::EMap, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::ENothing, country.GetDownloadedFiles(), ());

    country.RemoveOptions(TMapOptions::EMap);
    TEST_EQUAL(TMapOptions::ECarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::ECarRouting, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::ENothing, country.GetDownloadedFiles(), ());

    country.RemoveOptions(TMapOptions::ECarRouting);
    TEST_EQUAL(TMapOptions::ENothing, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::ENothing, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::ENothing, country.GetDownloadedFiles(), ());
  }

  {
    QueuedCountry country(index, TMapOptions::EMapWithCarRouting);
    TEST_EQUAL(TMapOptions::EMapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::EMap, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::ENothing, country.GetDownloadedFiles(), ());

    country.SwitchToNextFile();
    TEST_EQUAL(TMapOptions::EMapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::ECarRouting, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::EMap, country.GetDownloadedFiles(), ());

    country.RemoveOptions(TMapOptions::ECarRouting);
    TEST_EQUAL(TMapOptions::EMap, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::ENothing, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::EMap, country.GetDownloadedFiles(), ());
  }

  {
    QueuedCountry country(index, TMapOptions::EMapWithCarRouting);
    TEST_EQUAL(TMapOptions::EMapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::EMap, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::ENothing, country.GetDownloadedFiles(), ());

    country.SwitchToNextFile();
    TEST_EQUAL(TMapOptions::EMapWithCarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::ECarRouting, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::EMap, country.GetDownloadedFiles(), ());

    country.RemoveOptions(TMapOptions::EMap);
    TEST_EQUAL(TMapOptions::ECarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::ECarRouting, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::ENothing, country.GetDownloadedFiles(), ());

    country.SwitchToNextFile();
    TEST_EQUAL(TMapOptions::ECarRouting, country.GetInitOptions(), ());
    TEST_EQUAL(TMapOptions::ENothing, country.GetCurrentFile(), ());
    TEST_EQUAL(TMapOptions::ECarRouting, country.GetDownloadedFiles(), ());
  }
}

UNIT_TEST(QueuedCountry_Bits)
{
  Storage storage;
  TIndex const index = storage.FindIndexByFile("USA_Georgia");
  QueuedCountry country(index, TMapOptions::EMapWithCarRouting);
  TEST_EQUAL(TMapOptions::ENothing, country.GetDownloadedFiles(), ());

  TEST(country.SwitchToNextFile(), ());
  TEST_EQUAL(TMapOptions::EMap, country.GetDownloadedFiles(), ());

  TEST(!country.SwitchToNextFile(), ());
  TEST_EQUAL(TMapOptions::EMapWithCarRouting, country.GetDownloadedFiles(), ());
}
}  // namespace storage
