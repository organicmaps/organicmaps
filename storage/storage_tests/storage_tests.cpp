#include "testing/testing.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"
#include "storage/storage_defines.hpp"
#include "storage/storage_helpers.hpp"
#include "storage/storage_tests/fake_map_files_downloader.hpp"
#include "storage/storage_tests/task_runner.hpp"
#include "storage/storage_tests/test_map_files_downloader.hpp"

#include "indexer/indexer_tests/test_mwm_set.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "platform/platform_tests_support/scoped_dir.hpp"

#include "geometry/mercator.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "defines.hpp"

#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include "std/bind.hpp"
#include "std/condition_variable.hpp"
#include "std/map.hpp"
#include "std/mutex.hpp"
#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

#include <QtCore/QCoreApplication>

using namespace platform;

namespace storage
{
namespace
{
using TLocalFilePtr = shared_ptr<LocalCountryFile>;

string const kSingleMwmCountriesTxt =
    string(R"({
           "id": "Countries",
           "v": )" + strings::to_string(version::FOR_TESTING_SINGLE_MWM1) + R"(,
           "g": [
               {
                "id": "Abkhazia",
                "s": 4689718,
                "old": [
                 "Georgia"
                ]
               },
               {
                "id": "Algeria",
                "g": [
                 {
                  "id": "Algeria_Central",
                  "s": 24177144,
                  "old": [
                   "Algeria"
                  ]
                 },
                 {
                  "id": "Algeria_Coast",
                  "s": 66701534,
                  "old": [
                   "Algeria"
                  ]
                 }
                ]
               },
               {
                "id": "South Korea_South",
                "s": 48394664,
                "old": [
                 "South Korea"
                ]
               }
            ]})");

string const kTwoComponentMwmCountriesTxt =
    string(R"({
           "v": )" + strings::to_string(version::FOR_TESTING_TWO_COMPONENT_MWM1) + R"(,
           "n": "Countries",
           "g": [
            {
             "n":"Africa",
             "g":[
              {
               "n":"Algeria",
               "c":"dz",
               "s":33912897,
               "rs":56864398
              },
              {
               "n":"Angola",
               "c":"ao",
               "s":7384993,
               "rs":9429135
              }]
            },
            {
             "n":"Europe",
             "g":[
              {
               "n":"Albania",
               "c":"al",
               "s":9785225,
               "rs":4438392
              },
              {
               "n":"France",
               "c":"fr",
               "g":[
                {
                 "n":"Alsace",
                 "c":"fr",
                 "f":"France_Alsace",
                 "s":58811438,
                 "rs":9032707
                },
                {
                 "n":"Aquitaine",
                 "c":"fr",
                 "f":"France_Aquitaine",
                 "s":111693256,
                 "rs":32365165
                }]
               }
            ]}]})");

// This class checks steps Storage::DownloadMap() performs to download a map.
class CountryDownloaderChecker
{
public:
  CountryDownloaderChecker(Storage & storage, TCountryId const & countryId, MapOptions files,
                           vector<TStatus> const & transitionList)
      : m_storage(storage),
        m_countryId(countryId),
        m_countryFile(storage.GetCountryFile(m_countryId)),
        m_files(files),
        m_bytesDownloaded(0),
        m_totalBytesToDownload(0),
        m_slot(0),
        m_currStatus(0),
        m_transitionList(transitionList)
  {
    m_slot = m_storage.Subscribe(
        bind(&CountryDownloaderChecker::OnCountryStatusChanged, this, _1),
        bind(&CountryDownloaderChecker::OnCountryDownloadingProgress, this, _1, _2));
    TEST(storage.IsCoutryIdInCountryTree(countryId), (m_countryFile));
    TEST(!m_transitionList.empty(), (m_countryFile));
  }

  virtual ~CountryDownloaderChecker()
  {
    TEST_EQUAL(m_currStatus + 1, m_transitionList.size(), (m_countryFile));
    m_storage.Unsubscribe(m_slot);
  }

  void StartDownload()
  {
    TEST_EQUAL(0, m_currStatus, (m_countryFile));
    TEST_LESS(m_currStatus, m_transitionList.size(), (m_countryFile));
    TEST_EQUAL(m_transitionList[m_currStatus], m_storage.CountryStatusEx(m_countryId), (m_countryFile));
    m_storage.DownloadCountry(m_countryId, m_files);
  }

protected:
  virtual void OnCountryStatusChanged(TCountryId const & countryId)
  {
    if (countryId != m_countryId)
      return;

    TStatus const nextStatus = m_storage.CountryStatusEx(m_countryId);
    LOG(LINFO, (m_countryFile, "status transition: from", m_transitionList[m_currStatus], "to",
                nextStatus));
    TEST_LESS(m_currStatus + 1, m_transitionList.size(), (m_countryFile));
    TEST_EQUAL(nextStatus, m_transitionList[m_currStatus + 1], (m_countryFile));
    ++m_currStatus;
    if (m_transitionList[m_currStatus] == TStatus::EDownloading)
    {
      LocalAndRemoteSizeT localAndRemoteSize = m_storage.CountrySizeInBytes(m_countryId, m_files);
      m_totalBytesToDownload = localAndRemoteSize.second;
    }
  }

  virtual void OnCountryDownloadingProgress(TCountryId const & countryId,
                                            LocalAndRemoteSizeT const & progress)
  {
    if (countryId != m_countryId)
      return;

    LOG(LINFO, (m_countryFile, "downloading progress:", progress));

    TEST_GREATER(progress.first, m_bytesDownloaded, (m_countryFile));
    m_bytesDownloaded = progress.first;
    TEST_LESS_OR_EQUAL(m_bytesDownloaded, m_totalBytesToDownload, (m_countryFile));

    LocalAndRemoteSizeT localAndRemoteSize = m_storage.CountrySizeInBytes(m_countryId, m_files);
    TEST_EQUAL(m_totalBytesToDownload, localAndRemoteSize.second, (m_countryFile));
  }

  Storage & m_storage;
  TCountryId const m_countryId;
  CountryFile const m_countryFile;
  MapOptions const m_files;
  int64_t m_bytesDownloaded;
  int64_t m_totalBytesToDownload;
  int m_slot;

  size_t m_currStatus;
  vector<TStatus> m_transitionList;
};

class CancelDownloadingWhenAlmostDoneChecker : public CountryDownloaderChecker
{
public:
  CancelDownloadingWhenAlmostDoneChecker(Storage & storage, TCountryId const & countryId,
                                         TaskRunner & runner)
      : CountryDownloaderChecker(storage, countryId, MapOptions::Map,
                                 vector<TStatus>{TStatus::ENotDownloaded, TStatus::EDownloading,
                                                 TStatus::ENotDownloaded}),
        m_runner(runner)
  {
  }

protected:
  // CountryDownloaderChecker overrides:
  void OnCountryDownloadingProgress(TCountryId const & countryId,
                                    LocalAndRemoteSizeT const & progress) override
  {
    CountryDownloaderChecker::OnCountryDownloadingProgress(countryId, progress);

    // Cancel downloading when almost done.
    if (progress.first + 2 * FakeMapFilesDownloader::kBlockSize >= progress.second)
    {
      m_runner.PostTask([&]()
                        {
                          m_storage.DeleteFromDownloader(m_countryId);
                        });
    }
  }

  TaskRunner & m_runner;
};

// Checks following state transitions:
// NotDownloaded -> Downloading -> OnDisk.
unique_ptr<CountryDownloaderChecker> AbsentCountryDownloaderChecker(Storage & storage,
                                                                    TCountryId const & countryId,
                                                                    MapOptions files)
{
  return make_unique<CountryDownloaderChecker>(
      storage, countryId, files,
      vector<TStatus>{TStatus::ENotDownloaded, TStatus::EDownloading, TStatus::EOnDisk});
}

// Checks following state transitions:
// OnDisk -> Downloading -> OnDisk.
unique_ptr<CountryDownloaderChecker> PresentCountryDownloaderChecker(Storage & storage,
                                                                     TCountryId const & countryId,
                                                                     MapOptions files)
{
  return make_unique<CountryDownloaderChecker>(
      storage, countryId, files,
      vector<TStatus>{TStatus::EOnDisk, TStatus::EDownloading, TStatus::EOnDisk});
}

// Checks following state transitions:
// NotDownloaded -> InQueue -> Downloading -> OnDisk.
unique_ptr<CountryDownloaderChecker> QueuedCountryDownloaderChecker(Storage & storage,
                                                                    TCountryId const & countryId,
                                                                    MapOptions files)
{
  return make_unique<CountryDownloaderChecker>(
      storage, countryId, files, vector<TStatus>{TStatus::ENotDownloaded, TStatus::EInQueue,
                                             TStatus::EDownloading, TStatus::EOnDisk});
}

// Checks following state transitions:
// NotDownloaded -> Downloading -> NotDownloaded.
unique_ptr<CountryDownloaderChecker> CancelledCountryDownloaderChecker(Storage & storage,
                                                                       TCountryId const & countryId,
                                                                       MapOptions files)
{
  return make_unique<CountryDownloaderChecker>(
      storage, countryId, files,
      vector<TStatus>{TStatus::ENotDownloaded, TStatus::EDownloading, TStatus::ENotDownloaded});
}

class CountryStatusChecker
{
public:
  CountryStatusChecker(Storage & storage, TCountryId const & countryId, TStatus status)
      : m_storage(storage), m_countryId(countryId), m_status(status), m_triggered(false)
  {
    m_slot = m_storage.Subscribe(
        bind(&CountryStatusChecker::OnCountryStatusChanged, this, _1),
        bind(&CountryStatusChecker::OnCountryDownloadingProgress, this, _1, _2));
  }

  ~CountryStatusChecker()
  {
    TEST(m_triggered, ("Status checker wasn't triggered."));
    m_storage.Unsubscribe(m_slot);
  }

private:
  void OnCountryStatusChanged(TCountryId const & countryId)
  {
    if (countryId != m_countryId)
      return;
    TEST(!m_triggered, ("Status checker can be triggered only once."));
    TStatus status = m_storage.CountryStatusEx(m_countryId);
    TEST_EQUAL(m_status, status, ());
    m_triggered = true;
  }

  void OnCountryDownloadingProgress(TCountryId const & /* countryId */,
                                    LocalAndRemoteSizeT const & /* progress */)
  {
    TEST(false, ("Unexpected country downloading progress."));
  }

  Storage & m_storage;
  TCountryId const & m_countryId;
  TStatus m_status;
  bool m_triggered;
  int m_slot;
};

class FailedDownloadingWaiter
{
public:
  FailedDownloadingWaiter(Storage & storage, TCountryId const & countryId)
    : m_storage(storage), m_countryId(countryId), m_finished(false)
  {
    m_slot = m_storage.Subscribe(bind(&FailedDownloadingWaiter::OnStatusChanged, this, _1),
                                 bind(&FailedDownloadingWaiter::OnProgress, this, _1, _2));
  }

  ~FailedDownloadingWaiter()
  {
    Wait();
    m_storage.Unsubscribe(m_slot);
  }

  void Wait()
  {
    unique_lock<mutex> lock(m_mu);
    m_cv.wait(lock, [this]()
    {
      return m_finished;
    });
  }

  void OnStatusChanged(TCountryId const & countryId)
  {
    if (countryId != m_countryId)
      return;
    TStatus const status = m_storage.CountryStatusEx(countryId);
    if (status != TStatus::EDownloadFailed)
      return;
    lock_guard<mutex> lock(m_mu);
    m_finished = true;
    m_cv.notify_one();

    QCoreApplication::exit();
  }

  void OnProgress(TCountryId const & /* countryId */, LocalAndRemoteSizeT const & /* progress */) {}

private:
  Storage & m_storage;
  TCountryId const m_countryId;
  int m_slot;

  mutex m_mu;
  condition_variable m_cv;
  bool m_finished;
};

void OnCountryDownloaded(LocalCountryFile const & localFile)
{
  LOG(LINFO, ("OnCountryDownloaded:", localFile));
}

TLocalFilePtr CreateDummyMapFile(CountryFile const & countryFile, int64_t version, size_t size)
{
  TLocalFilePtr localFile = PreparePlaceForCountryFiles(version, string() /* dataDir */, countryFile);
  TEST(localFile.get(), ("Can't prepare place for", countryFile, "(version", version, ")"));
  {
    string const zeroes(size, '\0');
    FileWriter writer(localFile->GetPath(MapOptions::Map));
    writer.Write(zeroes.data(), zeroes.size());
  }
  localFile->SyncWithDisk();
  TEST_EQUAL(MapOptions::Map, localFile->GetFiles(), ());
  TEST_EQUAL(size, localFile->GetSize(MapOptions::Map), ());
  return localFile;
}

void InitStorage(Storage & storage, TaskRunner & runner,
                 Storage::TUpdate const & update = &OnCountryDownloaded)
{
  storage.Clear();
  storage.Init(update);
  storage.RegisterAllLocalMaps();
  storage.SetDownloaderForTesting(make_unique<FakeMapFilesDownloader>(runner));
}

unique_ptr<storage::CountryInfoGetter> CreateCountryInfoGetter(bool isSingleMwm)
{
  Platform & platform = GetPlatform();
  string const packedPolygons = isSingleMwm ? PACKED_POLYGONS_MIGRATE_FILE : PACKED_POLYGONS_FILE;
  string const countryTxt = isSingleMwm ? COUNTRIES_MIGRATE_FILE : COUNTRIES_FILE;
  return make_unique<storage::CountryInfoReader>(platform.GetReader(packedPolygons),
                                                 platform.GetReader(countryTxt));
}
}  // namespace

UNIT_TEST(StorageTest_Smoke)
{
  Storage storage;

  TCountryId const georgiaCountryId = storage.FindCountryIdByFile("Georgia");
  TEST(IsCountryIdValid(georgiaCountryId), ());
  CountryFile usaGeorgiaFile = storage.GetCountryFile(georgiaCountryId);
  TEST_EQUAL(platform::GetFileName(usaGeorgiaFile.GetName(), MapOptions::Map,
                                   version::FOR_TESTING_TWO_COMPONENT_MWM1),
             "Georgia" DATA_FILE_EXTENSION, ());

  if (version::IsSingleMwm(storage.GetCurrentDataVersion()))
    return; // Following code tests car routing files, and is not relevant for a single mwm case.

  TEST(IsCountryIdValid(georgiaCountryId), ());
  CountryFile georgiaFile = storage.GetCountryFile(georgiaCountryId);
  TEST_EQUAL(platform::GetFileName(georgiaFile.GetName(), MapOptions::CarRouting,
                                   version::FOR_TESTING_TWO_COMPONENT_MWM1),
             "Georgia" DATA_FILE_EXTENSION ROUTING_FILE_EXTENSION, ());
}

UNIT_TEST(StorageTest_SingleCountryDownloading)
{
  Storage storage;
  TaskRunner runner;
  InitStorage(storage, runner);

  TCountryId const azerbaijanCountryId = storage.FindCountryIdByFile("Azerbaijan");
  TEST(IsCountryIdValid(azerbaijanCountryId), ());

  CountryFile azerbaijanFile = storage.GetCountryFile(azerbaijanCountryId);
  storage.DeleteCountry(azerbaijanCountryId, MapOptions::Map);

  {
    MY_SCOPE_GUARD(cleanupCountryFiles,
                   bind(&Storage::DeleteCountry, &storage, azerbaijanCountryId, MapOptions::Map));
    unique_ptr<CountryDownloaderChecker> checker =
        AbsentCountryDownloaderChecker(storage, azerbaijanCountryId, MapOptions::Map);
    checker->StartDownload();
    runner.Run();
  }

  {
    MY_SCOPE_GUARD(cleanupCountryFiles, bind(&Storage::DeleteCountry, &storage, azerbaijanCountryId,
                                             MapOptions::Map));
    unique_ptr<CountryDownloaderChecker> checker =
        AbsentCountryDownloaderChecker(storage, azerbaijanCountryId, MapOptions::Map);
    checker->StartDownload();
    runner.Run();
  }
}

UNIT_TEST(StorageTest_TwoCountriesDownloading)
{
  Storage storage;
  TaskRunner runner;
  InitStorage(storage, runner);

  TCountryId const uruguayCountryId = storage.FindCountryIdByFile("Uruguay");
  TEST(IsCountryIdValid(uruguayCountryId), ());
  storage.DeleteCountry(uruguayCountryId, MapOptions::Map);
  MY_SCOPE_GUARD(cleanupUruguayFiles,
                 bind(&Storage::DeleteCountry, &storage, uruguayCountryId, MapOptions::Map));

  TCountryId const venezuelaCountryId = storage.FindCountryIdByFile("Venezuela");
  TEST(IsCountryIdValid(venezuelaCountryId), ());
  storage.DeleteCountry(venezuelaCountryId, MapOptions::Map);
  MY_SCOPE_GUARD(cleanupVenezuelaFiles, bind(&Storage::DeleteCountry, &storage, venezuelaCountryId,
                                             MapOptions::Map));

  unique_ptr<CountryDownloaderChecker> uruguayChecker =
      AbsentCountryDownloaderChecker(storage, uruguayCountryId, MapOptions::Map);
  unique_ptr<CountryDownloaderChecker> venezuelaChecker =
      QueuedCountryDownloaderChecker(storage, venezuelaCountryId, MapOptions::Map);
  uruguayChecker->StartDownload();
  venezuelaChecker->StartDownload();
  runner.Run();
}

UNIT_TEST(StorageTest_DeleteTwoVersionsOfTheSameCountry)
{
  Storage storage;
  bool const isSingleMwm = version::IsSingleMwm(storage.GetCurrentDataVersion());
  if (isSingleMwm)
    storage.SetCurrentDataVersionForTesting(version::FOR_TESTING_SINGLE_MWM_LATEST);
  int64_t const v1 = isSingleMwm ? version::FOR_TESTING_SINGLE_MWM1
                                 : version::FOR_TESTING_TWO_COMPONENT_MWM1;
  int64_t const v2 = isSingleMwm ? version::FOR_TESTING_SINGLE_MWM2
                                 : version::FOR_TESTING_TWO_COMPONENT_MWM2;

  storage.Init(&OnCountryDownloaded);
  storage.RegisterAllLocalMaps();

  TCountryId const countryId = storage.FindCountryIdByFile("Azerbaijan");
  TEST(IsCountryIdValid(countryId), ());
  CountryFile const countryFile = storage.GetCountryFile(countryId);

  storage.DeleteCountry(countryId, MapOptions::Map);
  TLocalFilePtr latestLocalFile = storage.GetLatestLocalFile(countryId);
  TEST(!latestLocalFile.get(), ("Country wasn't deleted from disk."));
  TEST_EQUAL(TStatus::ENotDownloaded, storage.CountryStatusEx(countryId), ());

  TLocalFilePtr localFileV1 = CreateDummyMapFile(countryFile, v1, 1024 /* size */);
  storage.RegisterAllLocalMaps();
  latestLocalFile = storage.GetLatestLocalFile(countryId);
  TEST(latestLocalFile.get(), ("Created map file wasn't found by storage."));
  TEST_EQUAL(latestLocalFile->GetVersion(), localFileV1->GetVersion(), ());
  TEST_EQUAL(TStatus::EOnDiskOutOfDate, storage.CountryStatusEx(countryId), ());

  TLocalFilePtr localFileV2 = CreateDummyMapFile(countryFile, v2, 2048 /* size */);
  storage.RegisterAllLocalMaps();
  latestLocalFile = storage.GetLatestLocalFile(countryId);
  TEST(latestLocalFile.get(), ("Created map file wasn't found by storage."));
  TEST_EQUAL(latestLocalFile->GetVersion(), localFileV2->GetVersion(), ());
  TEST_EQUAL(TStatus::EOnDiskOutOfDate, storage.CountryStatusEx(countryId), ());

  storage.DeleteCountry(countryId, MapOptions::Map);

  localFileV1->SyncWithDisk();
  TEST_EQUAL(MapOptions::Nothing, localFileV1->GetFiles(), ());

  localFileV2->SyncWithDisk();
  TEST_EQUAL(MapOptions::Nothing, localFileV2->GetFiles(), ());

  TEST_EQUAL(TStatus::ENotDownloaded, storage.CountryStatusEx(countryId), ());
}

UNIT_TEST(StorageTest_DownloadMapAndRoutingSeparately)
{
  Storage storage;
  bool const isSingleMwm = version::IsSingleMwm(storage.GetCurrentDataVersion());
  if (isSingleMwm)
    return;

  TaskRunner runner;
  tests::TestMwmSet mwmSet;
  InitStorage(storage, runner, [&mwmSet](LocalCountryFile const & localFile)
  {
    try
    {
      auto p = mwmSet.Register(localFile);
      TEST(p.first.IsAlive(), ());
    }
    catch (exception & e)
    {
      LOG(LERROR, ("Failed to register:", localFile, ":", e.what()));
    }
  });

  TCountryId const countryId = storage.FindCountryIdByFile("Azerbaijan");
  TEST(IsCountryIdValid(countryId), ());
  CountryFile const countryFile = storage.GetCountryFile(countryId);

  storage.DeleteCountry(countryId, MapOptions::Map);

  // Download map file only.
  {
    unique_ptr<CountryDownloaderChecker> checker =
        AbsentCountryDownloaderChecker(storage, countryId, MapOptions::Map);
    checker->StartDownload();
    runner.Run();
  }

  TLocalFilePtr localFileA = storage.GetLatestLocalFile(countryId);
  TEST(localFileA.get(), ());
  TEST_EQUAL(MapOptions::Map, localFileA->GetFiles(), ());

  MwmSet::MwmId id = mwmSet.GetMwmIdByCountryFile(countryFile);
  TEST(id.IsAlive(), ());
  TEST_EQUAL(MapOptions::Map, id.GetInfo()->GetLocalFile().GetFiles(), ());

  // Download routing file in addition to exising map file.
  {
    unique_ptr<CountryDownloaderChecker> checker =
        PresentCountryDownloaderChecker(storage, countryId, MapOptions::CarRouting);
    checker->StartDownload();
    runner.Run();
  }

  TLocalFilePtr localFileB = storage.GetLatestLocalFile(countryId);
  TEST(localFileB.get(), ());
  TEST_EQUAL(localFileA.get(), localFileB.get(), (*localFileA, *localFileB));
  TEST_EQUAL(MapOptions::MapWithCarRouting, localFileB->GetFiles(), ());

  TEST(id.IsAlive(), ());
  TEST_EQUAL(MapOptions::MapWithCarRouting, id.GetInfo()->GetLocalFile().GetFiles(), ());

  // Delete routing file and check status update.
  {
    CountryStatusChecker checker(storage, countryId, TStatus::EOnDisk);
    storage.DeleteCountry(countryId, MapOptions::CarRouting);
  }
  TLocalFilePtr localFileC = storage.GetLatestLocalFile(countryId);
  TEST(localFileC.get(), ());
  TEST_EQUAL(localFileB.get(), localFileC.get(), (*localFileB, *localFileC));
  TEST_EQUAL(MapOptions::Map, localFileC->GetFiles(), ());

  TEST(id.IsAlive(), ());
  TEST_EQUAL(MapOptions::Map, id.GetInfo()->GetLocalFile().GetFiles(), ());

  // Delete map file and check status update.
  {
    CountryStatusChecker checker(storage, countryId, TStatus::ENotDownloaded);
    storage.DeleteCountry(countryId, MapOptions::Map);
  }

  // Framework should notify MwmSet about deletion of a map file.
  // As there're no framework, there should not be any changes in MwmInfo.
  TEST(id.IsAlive(), ());
  TEST_EQUAL(MapOptions::Map, id.GetInfo()->GetLocalFile().GetFiles(), ());
}

UNIT_TEST(StorageTest_DeletePendingCountry)
{
  Storage storage;
  TaskRunner runner;
  InitStorage(storage, runner);

  TCountryId const countryId = storage.FindCountryIdByFile("Azerbaijan");
  TEST(IsCountryIdValid(countryId), ());
  storage.DeleteCountry(countryId, MapOptions::Map);

  {
    unique_ptr<CountryDownloaderChecker> checker =
        CancelledCountryDownloaderChecker(storage, countryId, MapOptions::Map);
    checker->StartDownload();
    storage.DeleteCountry(countryId, MapOptions::Map);
    runner.Run();
  }
}

UNIT_TEST(StorageTest_DownloadTwoCountriesAndDeleteSingleMwm)
{
  Storage storage;
  if (!version::IsSingleMwm(storage.GetCurrentDataVersion()))
    return;

  TaskRunner runner;
  InitStorage(storage, runner);

  TCountryId const uruguayCountryId = storage.FindCountryIdByFile("Uruguay");
  TEST(IsCountryIdValid(uruguayCountryId), ());
  storage.DeleteCountry(uruguayCountryId, MapOptions::Map);
  MY_SCOPE_GUARD(cleanupUruguayFiles, bind(&Storage::DeleteCountry, &storage, uruguayCountryId,
                                           MapOptions::Map));

  TCountryId const venezuelaCountryId = storage.FindCountryIdByFile("Venezuela");
  TEST(IsCountryIdValid(venezuelaCountryId), ());
  storage.DeleteCountry(venezuelaCountryId, MapOptions::Map);
  MY_SCOPE_GUARD(cleanupVenezuelaFiles, bind(&Storage::DeleteCountry, &storage, venezuelaCountryId,
                                             MapOptions::Map));

  {
    unique_ptr<CountryDownloaderChecker> uruguayChecker = make_unique<CountryDownloaderChecker>(
        storage, uruguayCountryId, MapOptions::Map,
        vector<TStatus>{TStatus::ENotDownloaded, TStatus::EDownloading, TStatus::EOnDisk});

    unique_ptr<CountryDownloaderChecker> venezuelaChecker = make_unique<CountryDownloaderChecker>(
        storage, venezuelaCountryId, MapOptions::Map,
        vector<TStatus>{TStatus::ENotDownloaded, TStatus::EInQueue,
                        TStatus::EDownloading, TStatus::EOnDisk});

    uruguayChecker->StartDownload();
    venezuelaChecker->StartDownload();
    runner.Run();
  }

  {
    unique_ptr<CountryDownloaderChecker> uruguayChecker = make_unique<CountryDownloaderChecker>(
        storage, uruguayCountryId, MapOptions::Map,
        vector<TStatus>{TStatus::EOnDisk, TStatus::ENotDownloaded});

    unique_ptr<CountryDownloaderChecker> venezuelaChecker = make_unique<CountryDownloaderChecker>(
        storage, venezuelaCountryId, MapOptions::Map,
        vector<TStatus>{TStatus::EOnDisk, TStatus::ENotDownloaded});

    storage.DeleteCountry(uruguayCountryId, MapOptions::Map);
    storage.DeleteCountry(venezuelaCountryId, MapOptions::Map);
    runner.Run();
  }

  TLocalFilePtr uruguayFile = storage.GetLatestLocalFile(uruguayCountryId);
  TEST(!uruguayFile.get(), (*uruguayFile));

  TLocalFilePtr venezuelaFile = storage.GetLatestLocalFile(venezuelaCountryId);
  TEST(!venezuelaFile.get(), ());
}

UNIT_TEST(StorageTest_DownloadTwoCountriesAndDeleteTwoComponentMwm)
{
  Storage storage;
  if (version::IsSingleMwm(storage.GetCurrentDataVersion()))
    return;

  TaskRunner runner;
  InitStorage(storage, runner);

  TCountryId const uruguayCountryId = storage.FindCountryIdByFile("Uruguay");
  TEST(IsCountryIdValid(uruguayCountryId), ());
  storage.DeleteCountry(uruguayCountryId, MapOptions::MapWithCarRouting);
  MY_SCOPE_GUARD(cleanupUruguayFiles, bind(&Storage::DeleteCountry, &storage, uruguayCountryId,
                                           MapOptions::MapWithCarRouting));

  TCountryId const venezuelaCountryId = storage.FindCountryIdByFile("Venezuela");
  TEST(IsCountryIdValid(venezuelaCountryId), ());
  storage.DeleteCountry(venezuelaCountryId, MapOptions::MapWithCarRouting);
  MY_SCOPE_GUARD(cleanupVenezuelaFiles, bind(&Storage::DeleteCountry, &storage, venezuelaCountryId,
                                             MapOptions::MapWithCarRouting));

  {
    // Map file will be deleted for Uruguay, thus, routing file should also be deleted. Therefore,
    // Uruguay should pass through following states: NotDownloaded -> Downloading -> NotDownloaded.
    unique_ptr<CountryDownloaderChecker> uruguayChecker = make_unique<CountryDownloaderChecker>(
        storage, uruguayCountryId, MapOptions::MapWithCarRouting,
        vector<TStatus>{TStatus::ENotDownloaded, TStatus::EDownloading, TStatus::ENotDownloaded});
    // Only routing file will be deleted for Venezuela, thus, Venezuela should pass through
    // following
    // states:
    // NotDownloaded -> InQueue (Venezuela is added after Uruguay) -> Downloading -> Downloading
    // (second notification will be sent after deletion of a routing file) -> OnDisk.
    unique_ptr<CountryDownloaderChecker> venezuelaChecker = make_unique<CountryDownloaderChecker>(
        storage, venezuelaCountryId, MapOptions::MapWithCarRouting,
        vector<TStatus>{TStatus::ENotDownloaded, TStatus::EInQueue, TStatus::EDownloading,
                        TStatus::EDownloading, TStatus::EOnDisk});
    uruguayChecker->StartDownload();
    venezuelaChecker->StartDownload();
    storage.DeleteCountry(uruguayCountryId, MapOptions::Map);
    storage.DeleteCountry(venezuelaCountryId, MapOptions::CarRouting);
    runner.Run();
  }
  // @TODO(bykoianko) This test changed its behaivier. This commented lines are left specially
  // to fixed later.
  TLocalFilePtr uruguayFile = storage.GetLatestLocalFile(uruguayCountryId);
  TEST(!uruguayFile.get(), (*uruguayFile));

  TLocalFilePtr venezuelaFile = storage.GetLatestLocalFile(venezuelaCountryId);
  TEST(venezuelaFile.get(), ());
  TEST_EQUAL(MapOptions::Map, venezuelaFile->GetFiles(), ());
}

UNIT_TEST(StorageTest_CancelDownloadingWhenAlmostDone)
{
  Storage storage;
  TaskRunner runner;
  InitStorage(storage, runner);

  TCountryId const countryId = storage.FindCountryIdByFile("Uruguay");
  TEST(IsCountryIdValid(countryId), ());
  storage.DeleteCountry(countryId, MapOptions::Map);
  MY_SCOPE_GUARD(cleanupFiles,
                 bind(&Storage::DeleteCountry, &storage, countryId, MapOptions::Map));

  {
    CancelDownloadingWhenAlmostDoneChecker checker(storage, countryId, runner);
    checker.StartDownload();
    runner.Run();
  }
  TLocalFilePtr file = storage.GetLatestLocalFile(countryId);
  TEST(!file, (*file));
}

UNIT_TEST(StorageTest_DeleteCountry)
{
  Storage storage;
  TaskRunner runner;
  InitStorage(storage, runner);

  tests_support::ScopedFile map("Wonderland.mwm", "map");
  LocalCountryFile file = LocalCountryFile::MakeForTesting("Wonderland");
  TEST_EQUAL(MapOptions::Map, file.GetFiles(), ());

  CountryIndexes::PreparePlaceOnDisk(file);
  string const bitsPath = CountryIndexes::GetPath(file, CountryIndexes::Index::Bits);
  {
    FileWriter writer(bitsPath);
    string const data = "bits";
    writer.Write(data.data(), data.size());
  }

  storage.RegisterFakeCountryFiles(file);
  TEST(map.Exists(), ());
  TEST(Platform::IsFileExistsByFullPath(bitsPath), (bitsPath));

  storage.DeleteCustomCountryVersion(file);
  TEST(!map.Exists(), ());
  TEST(!Platform::IsFileExistsByFullPath(bitsPath), (bitsPath));

  map.Reset();
}

UNIT_TEST(StorageTest_FailedDownloading)
{
  Storage storage;
  storage.Init(&OnCountryDownloaded);
  storage.SetDownloaderForTesting(make_unique<TestMapFilesDownloader>());
  storage.SetCurrentDataVersionForTesting(1234);

  TCountryId const countryId = storage.FindCountryIdByFile("Uruguay");
  CountryFile const countryFile = storage.GetCountryFile(countryId);

  // To prevent interference from other tests and on other tests it's
  // better to remove temprorary downloader files.
  DeleteDownloaderFilesForCountry(storage.GetCurrentDataVersion(), countryFile);
  MY_SCOPE_GUARD(cleanup, [&]()
  {
    DeleteDownloaderFilesForCountry(storage.GetCurrentDataVersion(), countryFile);
  });

  {
    FailedDownloadingWaiter waiter(storage, countryId);
    storage.DownloadCountry(countryId, MapOptions::Map);
    QCoreApplication::exec();
  }

  // File wasn't downloaded, but temprorary downloader files must exist.
  string const downloadPath =
      GetFileDownloadPath(storage.GetCurrentDataVersion(), countryFile, MapOptions::Map);
  TEST(!Platform::IsFileExistsByFullPath(downloadPath), ());
  TEST(Platform::IsFileExistsByFullPath(downloadPath + DOWNLOADING_FILE_EXTENSION), ());
  TEST(Platform::IsFileExistsByFullPath(downloadPath + RESUME_FILE_EXTENSION), ());
}

UNIT_TEST(StorageTest_ObsoleteMapsRemoval)
{
  Storage storage;
  CountryFile country("Azerbaijan");

  tests_support::ScopedDir dir1("1");
  tests_support::ScopedFile map1(dir1, country, MapOptions::Map, "map1");
  LocalCountryFile file1(dir1.GetFullPath(), country, 1 /* version */);
  CountryIndexes::PreparePlaceOnDisk(file1);

  tests_support::ScopedDir dir2("2");
  tests_support::ScopedFile map2(dir2, country, MapOptions::Map, "map2");
  LocalCountryFile file2(dir2.GetFullPath(), country, 2 /* version */);
  CountryIndexes::PreparePlaceOnDisk(file2);

  TEST(map1.Exists(), ());
  TEST(map2.Exists(), ());

  storage.RegisterAllLocalMaps();

  TEST(!map1.Exists(), ());
  map1.Reset();

  TEST(map2.Exists(), ());
}

UNIT_TEST(StorageTest_GetRootId)
{
  Storage storage(string(R"({
                           "id": "Countries",
                           "v": )" + strings::to_string(version::FOR_TESTING_SINGLE_MWM1) + R"(,
                           "g": []
                         })"), make_unique<TestMapFilesDownloader>());

  // The name of the root is the same for courntries.txt version 1 and version 2.
  TEST_EQUAL(storage.GetRootId(), "Countries", ());
}

UNIT_TEST(StorageTest_GetChildren)
{
  Storage storage(kSingleMwmCountriesTxt, make_unique<TestMapFilesDownloader>());

  TCountryId const world = storage.GetRootId();
  TEST_EQUAL(world, "Countries", ());

  TCountriesVec countriesList;
  storage.GetChildren(world, countriesList);
  TEST_EQUAL(countriesList.size(), 3, ());
  TEST_EQUAL(countriesList.front(), "Abkhazia", ());
  TEST_EQUAL(countriesList.back(), "South Korea_South", ());

  TCountriesVec abkhaziaList;
  storage.GetChildren("Abkhazia", abkhaziaList);
  TEST(abkhaziaList.empty(), ());

  TCountriesVec algeriaList;
  storage.GetChildren("Algeria", algeriaList);
  TEST_EQUAL(algeriaList.size(), 2, ());
  TEST_EQUAL(algeriaList.front(), "Algeria_Central", ());
}

UNIT_TEST(StorageTest_HasCountryId)
{
  TCountriesVec middleEarthCountryIdVec =
      {"Arnor", "Mordor", "Rhovanion", "Rhun", "Gondor", "Eriador", "Rohan"};
  sort(middleEarthCountryIdVec.begin(), middleEarthCountryIdVec.end());

  TEST(HasCountryId(middleEarthCountryIdVec, "Gondor"), ());
  TEST(HasCountryId(middleEarthCountryIdVec, "Arnor"), ());
  TEST(!HasCountryId(middleEarthCountryIdVec, "Azerbaijan"), ());
  TEST(!HasCountryId(middleEarthCountryIdVec, "Alban"), ());
}

UNIT_TEST(StorageTest_DownloadedMapTests)
{
  Storage storage;
  if (!version::IsSingleMwm(storage.GetCurrentDataVersion()))
    return; // storage::GetDownloadedChildren is not used in case of two components mwm.

  TaskRunner runner;
  InitStorage(storage, runner);

  TCountryId const algeriaCentralCountryId = storage.FindCountryIdByFile("Algeria_Central");
  TCountryId const algeriaCoastCountryId = storage.FindCountryIdByFile("Algeria_Coast");
  TEST(IsCountryIdValid(algeriaCentralCountryId), ());
  TEST(IsCountryIdValid(algeriaCoastCountryId), ());

  storage.DeleteCountry(algeriaCentralCountryId, MapOptions::Map);
  storage.DeleteCountry(algeriaCoastCountryId, MapOptions::Map);

  MY_SCOPE_GUARD(cleanupAlgeriaCentral,
                 bind(&Storage::DeleteCountry, &storage, algeriaCentralCountryId, MapOptions::Map));
  MY_SCOPE_GUARD(cleanupAlgeriaCoast,
                 bind(&Storage::DeleteCountry, &storage, algeriaCoastCountryId, MapOptions::Map));

  {
    auto algeriaCentralChecker = make_unique<CountryDownloaderChecker>(
        storage, algeriaCentralCountryId, MapOptions::Map,
        vector<TStatus>{TStatus::ENotDownloaded, TStatus::EDownloading, TStatus::EOnDisk});

    auto algeriaCoastChecker = make_unique<CountryDownloaderChecker>(
        storage, algeriaCoastCountryId, MapOptions::Map,
        vector<TStatus>{TStatus::ENotDownloaded, TStatus::EInQueue,
                        TStatus::EDownloading, TStatus::EOnDisk});

    algeriaCentralChecker->StartDownload();
    algeriaCoastChecker->StartDownload();
    runner.Run();
  }

  // Storage::GetLocalRealMaps() test.
  TCountriesVec localRealMaps;
  storage.GetLocalRealMaps(localRealMaps);
  sort(localRealMaps.begin(), localRealMaps.end());
  TEST(HasCountryId(localRealMaps, "Algeria_Central"), ());
  TEST(HasCountryId(localRealMaps, "Algeria_Coast"), ());
  TEST(!HasCountryId(localRealMaps, "Algeria_Coast.mwm"), ());
  TEST(!HasCountryId(localRealMaps, "World"), ());
  TEST(!HasCountryId(localRealMaps, "WorldCoasts"), ());

  TEST(storage.IsNodeDownloaded("Algeria_Central"), ());
  TEST(storage.IsNodeDownloaded("Algeria_Coast"), ());
  TEST(!storage.IsNodeDownloaded("Algeria_Coast.mwm"), ());
  TEST(!storage.IsNodeDownloaded("World"), ());
  TEST(!storage.IsNodeDownloaded("World"), ());

  // Storage::GetDownloadedChildren test when at least Algeria_Central and Algeria_Coast have been downloaded.
  TCountryId const rootCountryId = storage.GetRootId();
  TEST_EQUAL(rootCountryId, "Countries", ());
  TCountriesVec rootChildrenCountriesId;
  storage.GetDownloadedChildren(rootCountryId, rootChildrenCountriesId);
  sort(rootChildrenCountriesId.begin(), rootChildrenCountriesId.end());
  TEST(HasCountryId(rootChildrenCountriesId, "Algeria"), ());
  TEST(!HasCountryId(rootChildrenCountriesId, "Algeria_Central"), ());
  TEST(!HasCountryId(rootChildrenCountriesId, "Algeria_Coast"), ());

  TCountriesVec algeriaChildrenCountriesId;
  storage.GetDownloadedChildren("Algeria", algeriaChildrenCountriesId);
  sort(algeriaChildrenCountriesId.begin(), algeriaChildrenCountriesId.end());
  TEST(HasCountryId(algeriaChildrenCountriesId, "Algeria_Central"), ());
  TEST(HasCountryId(algeriaChildrenCountriesId, "Algeria_Coast"), ());

  TCountriesVec algeriaCentralChildrenCountriesId;
  storage.GetDownloadedChildren("Algeria_Central", algeriaCentralChildrenCountriesId);
  TEST(algeriaCentralChildrenCountriesId.empty(), ());

  storage.DeleteCountry(algeriaCentralCountryId, MapOptions::Map);
  // Storage::GetDownloadedChildren test when Algeria_Coast has been downloaded and
  // Algeria_Central has been deleted.
  TCountriesVec rootChildrenCountriesId2;
  storage.GetDownloadedChildren(rootCountryId, rootChildrenCountriesId2);
  sort(rootChildrenCountriesId2.begin(), rootChildrenCountriesId2.end());
  TEST(!HasCountryId(rootChildrenCountriesId2, "Algeria"), ());
  TEST(!HasCountryId(rootChildrenCountriesId2, "Algeria_Central"), ());
  TEST(HasCountryId(rootChildrenCountriesId2, "Algeria_Coast"), ());

  TCountriesVec childrenOfAbsentCountry;
  storage.GetDownloadedChildren("Algeria_Central", childrenOfAbsentCountry);
  TEST(childrenOfAbsentCountry.empty(), ());

  TCountriesVec algeriaCoastChildrenCountriesId;
  storage.GetDownloadedChildren("Algeria_Coast", algeriaCoastChildrenCountriesId);
  TEST(algeriaCoastChildrenCountriesId.empty(), ());

  TEST(!storage.IsNodeDownloaded("Algeria_Central"), ());
  TEST(storage.IsNodeDownloaded("Algeria_Coast"), ());

  storage.DeleteCountry(algeriaCoastCountryId, MapOptions::Map);
  // Storage::GetDownloadedChildren test when Algeria_Coast and Algeria_Central have been deleted.
  TCountriesVec rootChildrenCountriesId3;
  storage.GetDownloadedChildren(rootCountryId, rootChildrenCountriesId3);
  sort(rootChildrenCountriesId3.begin(), rootChildrenCountriesId3.end());
  TEST(!HasCountryId(rootChildrenCountriesId3, "Algeria"), ());
  TEST(!HasCountryId(rootChildrenCountriesId3, "Algeria_Central"), ());
  TEST(!HasCountryId(rootChildrenCountriesId3, "Algeria_Coast"), ());

  TEST(!storage.IsNodeDownloaded("Algeria_Central"), ());
  TEST(!storage.IsNodeDownloaded("Algeria_Coast"), ());
}

UNIT_TEST(StorageTest_IsPointCoveredByDownloadedMaps)
{
  Storage storage;
  TaskRunner runner;
  InitStorage(storage, runner);

  bool const isSingleMwm = version::IsSingleMwm(storage.GetCurrentDataVersion());
  auto const countryInfoGetter = CreateCountryInfoGetter(isSingleMwm);
  ASSERT(countryInfoGetter, ());
  string const uruguayId = string("Uruguay");
  m2::PointD const montevideoUruguay = MercatorBounds::FromLatLon(-34.8094, -56.1558);

  storage.DeleteCountry(uruguayId, MapOptions::Map);
  TEST(!IsPointCoveredByDownloadedMaps(montevideoUruguay, storage, *countryInfoGetter), ());

  {
    MY_SCOPE_GUARD(cleanupCountryFiles,
                   bind(&Storage::DeleteCountry, &storage, uruguayId, MapOptions::Map));
    auto const checker = AbsentCountryDownloaderChecker(storage, uruguayId, MapOptions::Map);
    checker->StartDownload();
    runner.Run();
    TEST(IsPointCoveredByDownloadedMaps(montevideoUruguay, storage, *countryInfoGetter), ());
  }
}

UNIT_TEST(StorageTest_TwoInstance)
{
  Platform & platform = GetPlatform();
  string const writableDir = platform.WritableDir();

  string const testDir1 = string("testdir1");
  Storage storage1(COUNTRIES_FILE, testDir1);
  platform::tests_support::ScopedDir removeTestDir1(testDir1);
  UNUSED_VALUE(removeTestDir1);
  string const versionDir1 =
      my::JoinFoldersToPath(testDir1, strings::to_string(storage1.GetCurrentDataVersion()));
  platform::tests_support::ScopedDir removeVersionDir1(versionDir1);
  UNUSED_VALUE(removeVersionDir1);
  TaskRunner runner1;
  InitStorage(storage1, runner1);

  string const testDir2 = string("testdir2");
  Storage storage2(COUNTRIES_MIGRATE_FILE, testDir2);
  platform::tests_support::ScopedDir removeTestDir2(testDir2);
  UNUSED_VALUE(removeTestDir2);
  string const versionDir2 =
      my::JoinFoldersToPath(testDir2, strings::to_string(storage2.GetCurrentDataVersion()));
  platform::tests_support::ScopedDir removeVersionDir2(versionDir2);
  UNUSED_VALUE(removeVersionDir2);
  TaskRunner runner2;
  InitStorage(storage2, runner2);

  string const uruguayId = string("Uruguay"); // This countyId is valid for single and two component mwms.
  storage1.DeleteCountry(uruguayId, MapOptions::Map);
  {
    MY_SCOPE_GUARD(cleanupCountryFiles,
                   bind(&Storage::DeleteCountry, &storage1, uruguayId, MapOptions::Map));
    auto const checker = AbsentCountryDownloaderChecker(storage1, uruguayId, MapOptions::Map);
    checker->StartDownload();
    runner1.Run();
    TEST(platform.IsFileExistsByFullPath(my::JoinFoldersToPath(writableDir, versionDir1)), ());
  }

  storage2.DeleteCountry(uruguayId, MapOptions::Map);
  {
    MY_SCOPE_GUARD(cleanupCountryFiles,
                   bind(&Storage::DeleteCountry, &storage2, uruguayId, MapOptions::Map));
    auto const checker = AbsentCountryDownloaderChecker(storage2, uruguayId, MapOptions::Map);
    checker->StartDownload();
    runner2.Run();
    TEST(platform.IsFileExistsByFullPath(my::JoinFoldersToPath(writableDir, versionDir1)), ());
  }
}

UNIT_TEST(StorageTest_ChildrenSizeSingleMwm)
{
  Storage storage(kSingleMwmCountriesTxt, make_unique<TestMapFilesDownloader>());

  Country const abkhaziaCountry = storage.CountryLeafByCountryId("Abkhazia");
  TEST_EQUAL(abkhaziaCountry.GetSubtreeMwmCounter(), 1, ());
  TEST_EQUAL(abkhaziaCountry.GetSubtreeMwmSizeBytes(), 4689718, ());

  Country const algeriaCountry = storage.CountryByCountryId("Algeria");
  TEST_EQUAL(algeriaCountry.GetSubtreeMwmCounter(), 2, ());
  TEST_EQUAL(algeriaCountry.GetSubtreeMwmSizeBytes(), 90878678, ());

  Country const southKoreaCountry = storage.CountryLeafByCountryId("South Korea_South");
  TEST_EQUAL(southKoreaCountry.GetSubtreeMwmCounter(), 1, ());
  TEST_EQUAL(southKoreaCountry.GetSubtreeMwmSizeBytes(), 48394664, ());
}

UNIT_TEST(StorageTest_GetNodeAttrsSingleMwm)
{
  Storage storage(kSingleMwmCountriesTxt, make_unique<TestMapFilesDownloader>());

  NodeAttrs nodeAttrs;
  storage.GetNodeAttrs("Abkhazia", nodeAttrs);
  TEST_EQUAL(nodeAttrs.m_mwmCounter, 1, ());
  TEST_EQUAL(nodeAttrs.m_mwmSize, 4689718, ());
  TEST_EQUAL(nodeAttrs.m_status, TStatus::ENotDownloaded, ());

  storage.GetNodeAttrs("Algeria", nodeAttrs);
  TEST_EQUAL(nodeAttrs.m_mwmCounter, 2, ());
  TEST_EQUAL(nodeAttrs.m_mwmSize, 90878678, ());
  TEST_EQUAL(nodeAttrs.m_status, TStatus::ENotDownloaded, ());

  storage.GetNodeAttrs("South Korea_South", nodeAttrs);
  TEST_EQUAL(nodeAttrs.m_mwmCounter, 1, ());
  TEST_EQUAL(nodeAttrs.m_mwmSize, 48394664, ());
  TEST_EQUAL(nodeAttrs.m_status, TStatus::ENotDownloaded, ());
}

}  // namespace storage
