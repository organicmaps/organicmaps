#include "testing/testing.hpp"

#include "storage/storage.hpp"
#include "storage/storage_defines.hpp"
#include "storage/storage_tests/fake_map_files_downloader.hpp"
#include "storage/storage_tests/task_runner.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "defines.hpp"

#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include "std/bind.hpp"
#include "std/map.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

using namespace platform;
using namespace storage;

namespace
{
// This class checks steps Storage::DownloadMap() performs to download a map.
class CountryDownloaderChecker
{
public:
  CountryDownloaderChecker(Storage & storage, TIndex const & index, TMapOptions files,
                           vector<TStatus> const & transitionList)
      : m_storage(storage),
        m_index(index),
        m_countryFile(storage.GetCountryFile(m_index)),
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
    TEST(m_index.IsValid(), (m_countryFile));
    TEST(!m_transitionList.empty(), (m_countryFile));
  }

  void StartDownload()
  {
    TEST_EQUAL(0, m_currStatus, (m_countryFile));
    TEST_LESS(m_currStatus, m_transitionList.size(), (m_countryFile));
    TEST_EQUAL(m_transitionList[m_currStatus], m_storage.CountryStatusEx(m_index),
                (m_countryFile));
    m_storage.DownloadCountry(m_index, m_files);
  }

  virtual ~CountryDownloaderChecker()
  {
    TEST_EQUAL(m_currStatus + 1, m_transitionList.size(), (m_countryFile));
    m_storage.Unsubscribe(m_slot);
  }

private:
  void OnCountryStatusChanged(TIndex const & index)
  {
    if (index != m_index)
      return;

    TStatus const nextStatus = m_storage.CountryStatusEx(m_index);
    LOG(LINFO, (m_countryFile, "status transition: from", m_transitionList[m_currStatus], "to",
                nextStatus));
    TEST_LESS(m_currStatus + 1, m_transitionList.size(), (m_countryFile));
    TEST_EQUAL(nextStatus, m_transitionList[m_currStatus + 1], (m_countryFile));
    ++m_currStatus;
    if (m_transitionList[m_currStatus] == TStatus::EDownloading)
    {
      LocalAndRemoteSizeT localAndRemoteSize = m_storage.CountrySizeInBytes(m_index, m_files);
      m_totalBytesToDownload = localAndRemoteSize.second;
    }
  }

  void OnCountryDownloadingProgress(TIndex const & index, LocalAndRemoteSizeT const & progress)
  {
    if (index != m_index)
      return;

    LOG(LINFO, (m_countryFile, "downloading progress:", progress));

    TEST_GREATER(progress.first, m_bytesDownloaded, (m_countryFile));
    m_bytesDownloaded = progress.first;
    TEST_LESS_OR_EQUAL(m_bytesDownloaded, m_totalBytesToDownload, (m_countryFile));

    LocalAndRemoteSizeT localAndRemoteSize = m_storage.CountrySizeInBytes(m_index, m_files);
    TEST_EQUAL(m_totalBytesToDownload, localAndRemoteSize.second, (m_countryFile));
  }

  Storage & m_storage;
  TIndex const m_index;
  CountryFile const m_countryFile;
  TMapOptions const m_files;
  int64_t m_bytesDownloaded;
  int64_t m_totalBytesToDownload;
  int m_slot;

  size_t m_currStatus;
  vector<TStatus> m_transitionList;
};

// Checks following state transitions:
// NotDownloaded -> Downloading -> OnDisk.
unique_ptr<CountryDownloaderChecker> AbsentCountryDownloaderChecker(Storage & storage,
                                                                    TIndex const & index,
                                                                    TMapOptions files)
{
  return make_unique<CountryDownloaderChecker>(
      storage, index, files,
      vector<TStatus>{TStatus::ENotDownloaded, TStatus::EDownloading, TStatus::EOnDisk});
}

// Checks following state transitions:
// OnDisk -> Downloading -> OnDisk.
unique_ptr<CountryDownloaderChecker> PresentCountryDownloaderChecker(Storage & storage,
                                                                     TIndex const & index,
                                                                     TMapOptions files)
{
  return make_unique<CountryDownloaderChecker>(
      storage, index, files,
      vector<TStatus>{TStatus::EOnDisk, TStatus::EDownloading, TStatus::EOnDisk});
}

// Checks following state transitions:
// NotDownloaded -> InQueue -> Downloading -> OnDisk.
unique_ptr<CountryDownloaderChecker> QueuedCountryDownloaderChecker(Storage & storage,
                                                                    TIndex const & index,
                                                                    TMapOptions files)
{
  return make_unique<CountryDownloaderChecker>(
      storage, index, files, vector<TStatus>{TStatus::ENotDownloaded, TStatus::EInQueue,
                                             TStatus::EDownloading, TStatus::EOnDisk});
}

// Checks following state transitions:
// NotDownloaded -> Downloading -> NotDownloaded.
unique_ptr<CountryDownloaderChecker> CancelledCountryDownloaderChecker(Storage & storage,
                                                                       TIndex const & index,
                                                                       TMapOptions files)
{
  return make_unique<CountryDownloaderChecker>(
      storage, index, files,
      vector<TStatus>{TStatus::ENotDownloaded, TStatus::EDownloading, TStatus::ENotDownloaded});
}

void OnCountryDownloaded(LocalCountryFile const & localFile)
{
  LOG(LINFO, ("OnCountryDownloaded:", localFile));
}

shared_ptr<LocalCountryFile> CreateDummyMapFile(CountryFile const & countryFile, int64_t version,
                                                size_t size)
{
  shared_ptr<LocalCountryFile> localFile =
      platform::PreparePlaceForCountryFiles(countryFile, version);
  TEST(localFile.get(), ("Can't prepare place for", countryFile, "(version ", version, ")"));
  {
    string const zeroes(size, '\0');
    FileWriter writer(localFile->GetPath(TMapOptions::EMap));
    writer.Write(zeroes.data(), zeroes.size());
  }
  localFile->SyncWithDisk();
  TEST_EQUAL(TMapOptions::EMap, localFile->GetFiles(), ());
  TEST_EQUAL(size, localFile->GetSize(TMapOptions::EMap), ());
  return localFile;
}

class CountryStatusChecker
{
public:
  CountryStatusChecker(Storage & storage, TIndex const & index, TStatus status)
      : m_storage(storage), m_index(index), m_status(status), m_triggered(false)
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
  void OnCountryStatusChanged(TIndex const & index)
  {
    if (index != m_index)
      return;
    TEST(!m_triggered, ("Status checker can be triggered only once."));
    TStatus status = m_storage.CountryStatusEx(m_index);
    TEST_EQUAL(m_status, status, ());
    m_triggered = true;
  }

  void OnCountryDownloadingProgress(TIndex const & /* index */,
                                    LocalAndRemoteSizeT const & /* progress */)
  {
    TEST(false, ("Unexpected country downloading progress."));
  }

  Storage & m_storage;
  TIndex const & m_index;
  TStatus m_status;
  bool m_triggered;
  int m_slot;
};

void InitStorage(Storage & storage, TaskRunner & runner)
{
  storage.Init(&OnCountryDownloaded);
  storage.RegisterAllLocalMaps();
  storage.SetDownloaderForTesting(make_unique<FakeMapFilesDownloader>(runner));
}
}  // namespace

UNIT_TEST(StorageTest_Smoke)
{
  Storage storage;

  TIndex const usaGeorgiaIndex = storage.FindIndexByFile("USA_Georgia");
  TEST(usaGeorgiaIndex.IsValid(), ());
  CountryFile usaGeorgiaFile = storage.GetCountryFile(usaGeorgiaIndex);
  TEST_EQUAL(usaGeorgiaFile.GetNameWithExt(TMapOptions::EMap), "USA_Georgia" DATA_FILE_EXTENSION,
             ());

  TIndex const georgiaIndex = storage.FindIndexByFile("Georgia");
  TEST(georgiaIndex.IsValid(), ());
  CountryFile georgiaFile = storage.GetCountryFile(georgiaIndex);
  TEST_EQUAL(georgiaFile.GetNameWithExt(TMapOptions::ECarRouting),
             "Georgia" DATA_FILE_EXTENSION ROUTING_FILE_EXTENSION, ());

  TEST_NOT_EQUAL(usaGeorgiaIndex, georgiaIndex, ());
}

UNIT_TEST(StorageTest_SingleCountryDownloading)
{
  Storage storage;
  TaskRunner runner;
  InitStorage(storage, runner);

  TIndex const azerbaijanIndex = storage.FindIndexByFile("Azerbaijan");
  TEST(azerbaijanIndex.IsValid(), ());

  CountryFile azerbaijanFile = storage.GetCountryFile(azerbaijanIndex);
  storage.DeleteCountry(azerbaijanIndex, TMapOptions::EMapWithCarRouting);

  {
    MY_SCOPE_GUARD(cleanupCountryFiles,
                   bind(&Storage::DeleteCountry, &storage, azerbaijanIndex, TMapOptions::EMap));
    unique_ptr<CountryDownloaderChecker> checker =
        AbsentCountryDownloaderChecker(storage, azerbaijanIndex, TMapOptions::EMapWithCarRouting);
    checker->StartDownload();
    runner.Run();
  }

  {
    MY_SCOPE_GUARD(cleanupCountryFiles, bind(&Storage::DeleteCountry, &storage, azerbaijanIndex,
                                             TMapOptions::EMapWithCarRouting));
    unique_ptr<CountryDownloaderChecker> checker =
        AbsentCountryDownloaderChecker(storage, azerbaijanIndex, TMapOptions::EMapWithCarRouting);
    checker->StartDownload();
    runner.Run();
  }
}

UNIT_TEST(StorageTest_TwoCountriesDownloading)
{
  Storage storage;
  TaskRunner runner;
  InitStorage(storage, runner);

  TIndex const uruguayIndex = storage.FindIndexByFile("Uruguay");
  TEST(uruguayIndex.IsValid(), ());
  storage.DeleteCountry(uruguayIndex, TMapOptions::EMap);
  MY_SCOPE_GUARD(cleanupUruguayFiles,
                 bind(&Storage::DeleteCountry, &storage, uruguayIndex, TMapOptions::EMap));

  TIndex const venezuelaIndex = storage.FindIndexByFile("Venezuela");
  TEST(venezuelaIndex.IsValid(), ());
  storage.DeleteCountry(venezuelaIndex, TMapOptions::EMapWithCarRouting);
  MY_SCOPE_GUARD(cleanupVenezuelaFiles, bind(&Storage::DeleteCountry, &storage, venezuelaIndex,
                                             TMapOptions::EMapWithCarRouting));

  unique_ptr<CountryDownloaderChecker> uruguayChecker =
      AbsentCountryDownloaderChecker(storage, uruguayIndex, TMapOptions::EMap);
  unique_ptr<CountryDownloaderChecker> venezuelaChecker =
      QueuedCountryDownloaderChecker(storage, venezuelaIndex, TMapOptions::EMapWithCarRouting);
  uruguayChecker->StartDownload();
  venezuelaChecker->StartDownload();
  runner.Run();
}

UNIT_TEST(StorageTest_DeleteTwoVersionsOfTheSameCountry)
{
  Storage storage;
  storage.Init(&OnCountryDownloaded);
  storage.RegisterAllLocalMaps();

  TIndex const index = storage.FindIndexByFile("Azerbaijan");
  TEST(index.IsValid(), ());
  CountryFile const countryFile = storage.GetCountryFile(index);

  storage.DeleteCountry(index, TMapOptions::EMapWithCarRouting);
  shared_ptr<LocalCountryFile> latestLocalFile = storage.GetLatestLocalFile(index);
  TEST(!latestLocalFile.get(), ("Country wasn't deleted from disk."));
  TEST_EQUAL(TStatus::ENotDownloaded, storage.CountryStatusEx(index), ());

  shared_ptr<LocalCountryFile> localFileV1 =
      CreateDummyMapFile(countryFile, 1 /* version */, 1024 /* size */);
  storage.RegisterAllLocalMaps();
  latestLocalFile = storage.GetLatestLocalFile(index);
  TEST(latestLocalFile.get(), ("Created map file wasn't found by storage."));
  TEST_EQUAL(latestLocalFile->GetVersion(), localFileV1->GetVersion(), ());
  TEST_EQUAL(TStatus::EOnDiskOutOfDate, storage.CountryStatusEx(index), ());

  shared_ptr<LocalCountryFile> localFileV2 =
      CreateDummyMapFile(countryFile, 2 /* version */, 2048 /* size */);
  storage.RegisterAllLocalMaps();
  latestLocalFile = storage.GetLatestLocalFile(index);
  TEST(latestLocalFile.get(), ("Created map file wasn't found by storage."));
  TEST_EQUAL(latestLocalFile->GetVersion(), localFileV2->GetVersion(), ());
  TEST_EQUAL(TStatus::EOnDiskOutOfDate, storage.CountryStatusEx(index), ());

  storage.DeleteCountry(index, TMapOptions::EMap);

  localFileV1->SyncWithDisk();
  TEST_EQUAL(TMapOptions::ENothing, localFileV1->GetFiles(), ());

  localFileV2->SyncWithDisk();
  TEST_EQUAL(TMapOptions::ENothing, localFileV2->GetFiles(), ());

  TEST_EQUAL(TStatus::ENotDownloaded, storage.CountryStatusEx(index), ());
}

UNIT_TEST(StorageTest_DownloadCountryAndDeleteRoutingOnly)
{
  Storage storage;
  TaskRunner runner;
  InitStorage(storage, runner);

  TIndex const index = storage.FindIndexByFile("Azerbaijan");
  TEST(index.IsValid(), ());
  storage.DeleteCountry(index, TMapOptions::EMapWithCarRouting);

  {
    unique_ptr<CountryDownloaderChecker> checker =
        AbsentCountryDownloaderChecker(storage, index, TMapOptions::EMapWithCarRouting);
    checker->StartDownload();
    runner.Run();
  }

  // Delete routing file only and check that latest local file wasn't changed.
  shared_ptr<LocalCountryFile> localFileA = storage.GetLatestLocalFile(index);
  TEST(localFileA.get(), ());
  TEST_EQUAL(TMapOptions::EMapWithCarRouting, localFileA->GetFiles(), ());

  storage.DeleteCountry(index, TMapOptions::ECarRouting);

  shared_ptr<LocalCountryFile> localFileB = storage.GetLatestLocalFile(index);
  TEST(localFileB.get(), ());
  TEST_EQUAL(localFileA.get(), localFileB.get(), (*localFileA, *localFileB));
  TEST_EQUAL(TMapOptions::EMap, localFileB->GetFiles(), ());

  storage.DeleteCountry(index, TMapOptions::EMap);
  shared_ptr<LocalCountryFile> localFileC = storage.GetLatestLocalFile(index);
  TEST(!localFileC.get(), (*localFileC));
}

UNIT_TEST(StorageTest_DownloadMapAndRoutingSeparately)
{
  Storage storage;
  TaskRunner runner;
  InitStorage(storage, runner);

  TIndex const index = storage.FindIndexByFile("Azerbaijan");
  TEST(index.IsValid(), ());
  storage.DeleteCountry(index, TMapOptions::EMapWithCarRouting);

  // Download map file only.
  {
    unique_ptr<CountryDownloaderChecker> checker =
        AbsentCountryDownloaderChecker(storage, index, TMapOptions::EMap);
    checker->StartDownload();
    runner.Run();
  }

  shared_ptr<LocalCountryFile> localFileA = storage.GetLatestLocalFile(index);
  TEST(localFileA.get(), ());
  TEST_EQUAL(TMapOptions::EMap, localFileA->GetFiles(), ());

  // Download routing file in addition to exising map file.
  {
    unique_ptr<CountryDownloaderChecker> checker =
        PresentCountryDownloaderChecker(storage, index, TMapOptions::ECarRouting);
    checker->StartDownload();
    runner.Run();
  }

  shared_ptr<LocalCountryFile> localFileB = storage.GetLatestLocalFile(index);
  TEST(localFileB.get(), ());
  TEST_EQUAL(localFileA.get(), localFileB.get(), (*localFileA, *localFileB));
  TEST_EQUAL(TMapOptions::EMapWithCarRouting, localFileB->GetFiles(), ());

  // Delete routing file and check status update.
  {
    CountryStatusChecker checker(storage, index, TStatus::EOnDisk);
    storage.DeleteCountry(index, TMapOptions::ECarRouting);
  }
  shared_ptr<LocalCountryFile> localFileC = storage.GetLatestLocalFile(index);
  TEST(localFileC.get(), ());
  TEST_EQUAL(localFileB.get(), localFileC.get(), (*localFileB, *localFileC));
  TEST_EQUAL(TMapOptions::EMap, localFileC->GetFiles(), ());

  // Delete map file and check status update.
  {
    CountryStatusChecker checker(storage, index, TStatus::ENotDownloaded);
    storage.DeleteCountry(index, TMapOptions::EMap);
  }
}

UNIT_TEST(StorageTest_DeletePendingCountry)
{
  Storage storage;
  TaskRunner runner;
  InitStorage(storage, runner);

  TIndex const index = storage.FindIndexByFile("Azerbaijan");
  TEST(index.IsValid(), ());
  storage.DeleteCountry(index, TMapOptions::EMapWithCarRouting);

  {
    unique_ptr<CountryDownloaderChecker> checker =
        CancelledCountryDownloaderChecker(storage, index, TMapOptions::EMap);
    checker->StartDownload();
    storage.DeleteCountry(index, TMapOptions::EMapWithCarRouting);
    runner.Run();
  }
}

UNIT_TEST(StorageTest_DownloadTwoCountriesAndDelete)
{
  Storage storage;
  TaskRunner runner;
  InitStorage(storage, runner);

  TIndex const uruguayIndex = storage.FindIndexByFile("Uruguay");
  TEST(uruguayIndex.IsValid(), ());
  storage.DeleteCountry(uruguayIndex, TMapOptions::EMapWithCarRouting);
  MY_SCOPE_GUARD(cleanupUruguayFiles, bind(&Storage::DeleteCountry, &storage, uruguayIndex,
                                           TMapOptions::EMapWithCarRouting));

  TIndex const venezuelaIndex = storage.FindIndexByFile("Venezuela");
  TEST(venezuelaIndex.IsValid(), ());
  storage.DeleteCountry(venezuelaIndex, TMapOptions::EMapWithCarRouting);
  MY_SCOPE_GUARD(cleanupVenezuelaFiles, bind(&Storage::DeleteCountry, &storage, venezuelaIndex,
                                             TMapOptions::EMapWithCarRouting));

  {
    // Map file will be deleted for Uruguay, thus, routing file should also be deleted. Therefore,
    // Uruguay should pass through following states: NotDownloaded -> Downloading -> NotDownloaded.
    unique_ptr<CountryDownloaderChecker> uruguayChecker = make_unique<CountryDownloaderChecker>(
        storage, uruguayIndex, TMapOptions::EMapWithCarRouting,
        vector<TStatus>{TStatus::ENotDownloaded, TStatus::EDownloading, TStatus::ENotDownloaded});
    // Only routing file will be deleted for Venezuela, thus, Venezuela should pass through
    // following
    // states:
    // NotDownloaded -> InQueue (Venezuela is added after Uruguay) -> Downloading -> Downloading
    // (second notification will be sent after deletion of a routing file) -> OnDisk.
    unique_ptr<CountryDownloaderChecker> venezuelaChecker = make_unique<CountryDownloaderChecker>(
        storage, venezuelaIndex, TMapOptions::EMapWithCarRouting,
        vector<TStatus>{TStatus::ENotDownloaded, TStatus::EInQueue, TStatus::EDownloading,
                        TStatus::EDownloading, TStatus::EOnDisk});
    uruguayChecker->StartDownload();
    venezuelaChecker->StartDownload();
    storage.DeleteCountry(uruguayIndex, TMapOptions::EMap);
    storage.DeleteCountry(venezuelaIndex, TMapOptions::ECarRouting);
    runner.Run();
  }
  shared_ptr<LocalCountryFile> uruguayFile = storage.GetLatestLocalFile(uruguayIndex);
  TEST(!uruguayFile.get(), (*uruguayFile));

  shared_ptr<LocalCountryFile> venezuelaFile = storage.GetLatestLocalFile(venezuelaIndex);
  TEST(venezuelaFile.get(), ());
  TEST_EQUAL(TMapOptions::EMap, venezuelaFile->GetFiles(), ());
}
