#include "testing/testing.hpp"

#include "storage/storage.hpp"
#include "storage/storage_defines.hpp"
#include "storage/storage_tests/fake_map_files_downloader.hpp"
#include "storage/storage_tests/task_runner.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "defines.hpp"

#include "base/scope_guard.hpp"

#include "std/bind.hpp"
#include "std/unique_ptr.hpp"

using namespace storage;

namespace
{
class CountryDownloaderChecker
{
public:
  CountryDownloaderChecker(Storage & storage, string const & countryFileName, TMapOptions files)
      : m_storage(storage),
        m_index(m_storage.FindIndexByFile(countryFileName)),
        m_files(files),
        m_lastStatus(TStatus::ENotDownloaded),
        m_bytesDownloaded(0),
        m_totalBytesToDownload(0),
        m_slot(0)
  {
    m_slot = m_storage.Subscribe(
        bind(&CountryDownloaderChecker::OnCountryStatusChanged, this, _1),
        bind(&CountryDownloaderChecker::OnCountryDownloadingProgress, this, _1, _2));
    CHECK(m_index.IsValid(), ());
  }

  void StartDownload()
  {
    CHECK_EQUAL(m_lastStatus, m_storage.CountryStatusEx(m_index), ());
    m_storage.DownloadCountry(m_index, m_files);
  }

  virtual ~CountryDownloaderChecker()
  {
    m_storage.Unsubscribe(m_slot);

    CHECK_EQUAL(TStatus::EOnDisk, m_lastStatus, ());
    CHECK_EQUAL(m_bytesDownloaded, m_totalBytesToDownload, ());

    LocalAndRemoteSizeT localAndRemoteSize = m_storage.CountrySizeInBytes(m_index, m_files);
    CHECK_EQUAL(m_bytesDownloaded, localAndRemoteSize.first, ());
    CHECK_EQUAL(m_totalBytesToDownload, localAndRemoteSize.second, ());
  }

protected:
  virtual void CheckStatusTransition(TStatus oldStatus, TStatus newStatus) const = 0;

private:
  void OnCountryStatusChanged(TIndex const & index)
  {
    if (index != m_index)
      return;
    TStatus status = m_storage.CountryStatusEx(m_index);

    CheckStatusTransition(m_lastStatus, status);
    if (status == TStatus::EDownloading)
    {
      LocalAndRemoteSizeT localAndRemoteSize = m_storage.CountrySizeInBytes(m_index, m_files);
      m_totalBytesToDownload = localAndRemoteSize.second;
    }
    m_lastStatus = status;
  }

  void OnCountryDownloadingProgress(TIndex const & index, LocalAndRemoteSizeT const & progress)
  {
    if (index != m_index)
      return;

    CHECK_GREATER(progress.first, m_bytesDownloaded, ());
    m_bytesDownloaded = progress.first;
    CHECK_LESS_OR_EQUAL(m_bytesDownloaded, m_totalBytesToDownload, ());

    LocalAndRemoteSizeT localAndRemoteSize = m_storage.CountrySizeInBytes(m_index, m_files);
    CHECK_EQUAL(m_totalBytesToDownload, localAndRemoteSize.second, ());
  }

  Storage & m_storage;
  TIndex const m_index;

  TMapOptions const m_files;
  TStatus m_lastStatus;
  int64_t m_bytesDownloaded;
  int64_t m_totalBytesToDownload;
  int m_slot;
};

// Checks following state transitions:
// NotDownloaded -> Downloading -> OnDisk.
class AbsentCountryDownloaderChecker : public CountryDownloaderChecker
{
public:
  AbsentCountryDownloaderChecker(Storage & storage, string const & countryFileName,
                                 TMapOptions files)
      : CountryDownloaderChecker(storage, countryFileName, files)
  {
  }

  ~AbsentCountryDownloaderChecker() override = default;

protected:
  void CheckStatusTransition(TStatus oldStatus, TStatus newStatus) const override
  {
    switch (newStatus)
    {
      case TStatus::EDownloading:
        CHECK_EQUAL(oldStatus, TStatus::ENotDownloaded,
                    ("It's only possible to move from NotDownloaded to Downloading."));
        break;
      case TStatus::EOnDisk:
        CHECK_EQUAL(oldStatus, TStatus::EDownloading,
                    ("It's only possible to move from Downloading to OnDisk."));
        break;
      default:
        CHECK(false, ("Unknown state change: from", oldStatus, "to", newStatus));
    }
  }
};

// Checks following state transitions:
// NotDownloaded -> InQueue -> Downloading -> OnDisk.
class QueuedCountryDownloaderChecker : public CountryDownloaderChecker
{
public:
  QueuedCountryDownloaderChecker(Storage & storage, string const & countryFileName,
                                 TMapOptions files)
      : CountryDownloaderChecker(storage, countryFileName, files)
  {
  }

  ~QueuedCountryDownloaderChecker() override = default;

protected:
  void CheckStatusTransition(TStatus oldStatus, TStatus newStatus) const override
  {
    switch (newStatus)
    {
      case TStatus::EInQueue:
        CHECK_EQUAL(oldStatus, TStatus::ENotDownloaded,
                    ("It's only possible to move from NotDownloaded to InQueue."));
        break;
      case TStatus::EDownloading:
        CHECK_EQUAL(oldStatus, TStatus::EInQueue,
                    ("It's only possible to move from InQueue to Downloading."));
        break;
      case TStatus::EOnDisk:
        CHECK_EQUAL(oldStatus, TStatus::EDownloading,
                    ("It's only possible to move from Downloading to OnDisk."));
        break;
      default:
        CHECK(false, ("Unknown state change: from", oldStatus, "to", newStatus));
    }
  }
};

// Removes country's files that can be created during and after downloading.
void CleanupCountryFiles(string const & countryFileName)
{
  Platform & platform = GetPlatform();

  string const localMapFile =
      my::JoinFoldersToPath(platform.WritableDir(), countryFileName + DATA_FILE_EXTENSION);
  my::DeleteFileX(localMapFile);
  my::DeleteFileX(localMapFile + READY_FILE_EXTENSION);

  string const localRoutingFile = localMapFile + ROUTING_FILE_EXTENSION;
  my::DeleteFileX(localRoutingFile);
  my::DeleteFileX(localRoutingFile + READY_FILE_EXTENSION);
}

void OnCountryDownloaded(string const & mapFileName, TMapOptions files)
{
  Platform & platform = GetPlatform();

  string const localMapFile = my::JoinFoldersToPath(platform.WritableDir(), mapFileName);
  string const localRoutingFile = localMapFile + ROUTING_FILE_EXTENSION;

  if (files & TMapOptions::EMap)
    CHECK(my::RenameFileX(localMapFile + READY_FILE_EXTENSION, localMapFile), ());
  if (files & TMapOptions::ECarRouting)
    CHECK(my::RenameFileX(localRoutingFile + READY_FILE_EXTENSION, localRoutingFile), ());
}

}  // namespace

UNIT_TEST(StorageTest_Smoke)
{
  Storage st;

  TIndex const i1 = st.FindIndexByFile("USA_Georgia");
  TEST(i1.IsValid(), ());
  TEST_EQUAL(st.CountryFileName(i1, TMapOptions::EMap), "USA_Georgia" DATA_FILE_EXTENSION, ());

  TIndex const i2 = st.FindIndexByFile("Georgia");
  TEST(i2.IsValid(), ());
  TEST_EQUAL(st.CountryFileName(i2, TMapOptions::ECarRouting),
             "Georgia" DATA_FILE_EXTENSION ROUTING_FILE_EXTENSION, ());

  TEST_NOT_EQUAL(i1, i2, ());
}

UNIT_TEST(StorageTest_SingleCountryDownloading)
{
  string const azerbaijanFileName = "Azerbaijan";
  CleanupCountryFiles(azerbaijanFileName);

  Storage storage;
  storage.Init(&OnCountryDownloaded);

  TaskRunner taskRunner;
  storage.SetDownloaderForTesting(make_unique<FakeMapFilesDownloader>(taskRunner));

  {
    MY_SCOPE_GUARD(cleanupCountryFiles, bind(&CleanupCountryFiles, azerbaijanFileName));
    AbsentCountryDownloaderChecker checker(storage, azerbaijanFileName, TMapOptions::EMap);
    checker.StartDownload();
    taskRunner.Run();
  }

  {
    MY_SCOPE_GUARD(cleanupCountryFiles, bind(&CleanupCountryFiles, azerbaijanFileName));
    AbsentCountryDownloaderChecker checker(storage, azerbaijanFileName,
                                           TMapOptions::EMapWithCarRouting);
    checker.StartDownload();
    taskRunner.Run();
  }
}

UNIT_TEST(StorageTest_TwoCountriesDownloading)
{
  string const uruguayFileName = "Uruguay";
  string const venezuelaFileName = "Venezuela";
  CleanupCountryFiles(uruguayFileName);
  MY_SCOPE_GUARD(cleanupUruguayFiles, bind(&CleanupCountryFiles, uruguayFileName));

  CleanupCountryFiles(venezuelaFileName);
  MY_SCOPE_GUARD(cleanupVenezuelaFiles, bind(&CleanupCountryFiles, venezuelaFileName));

  Storage storage;
  storage.Init(&OnCountryDownloaded);

  TaskRunner taskRunner;
  storage.SetDownloaderForTesting(make_unique<FakeMapFilesDownloader>(taskRunner));

  AbsentCountryDownloaderChecker uruguayChecker(storage, uruguayFileName, TMapOptions::EMap);
  QueuedCountryDownloaderChecker venezuelaChecker(storage, venezuelaFileName,
                                                  TMapOptions::EMapWithCarRouting);
  uruguayChecker.StartDownload();
  venezuelaChecker.StartDownload();
  taskRunner.Run();
}
