#include "testing/testing.hpp"

#include "storage/storage.hpp"
#include "storage/storage_defines.hpp"
#include "storage/storage_tests/fake_map_files_downloader.hpp"
#include "storage/storage_tests/message_loop.hpp"

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
// This class checks steps Storage::DownloadMap() performs to download a map.
class CountryDownloaderChecker
{
public:
  CountryDownloaderChecker(Storage & storage, string const & countryFileName)
      : m_storage(storage),
        m_index(m_storage.FindIndexByFile(countryFileName)),
        m_lastStatus(TStatus::ENotDownloaded),
        m_bytesDownloaded(0),
        m_totalBytesToDownload(-1)
  {
    CHECK(m_index.IsValid(), ());
    CHECK_EQUAL(m_lastStatus, m_storage.CountryStatusEx(m_index), ());
    m_slot = m_storage.Subscribe(
        bind(&CountryDownloaderChecker::OnCountryStatusChanged, this, _1),
        bind(&CountryDownloaderChecker::OnCountryDownloadingProgress, this, _1, _2));

    m_storage.DownloadCountry(m_index, TMapOptions::EMap);
  }

  ~CountryDownloaderChecker()
  {
    CHECK_EQUAL(TStatus::EOnDisk, m_lastStatus, ());
    CHECK_EQUAL(m_bytesDownloaded, m_totalBytesToDownload, ());
    m_storage.Unsubscribe(m_slot);
  }

private:
  void OnCountryStatusChanged(TIndex const & index)
  {
    if (index != m_index)
      return;
    TStatus status = m_storage.CountryStatusEx(m_index);

    LOG(LINFO, ("OnCountryStatusChanged", status));
    switch (status)
    {
      case TStatus::EDownloading:
        CHECK(m_lastStatus == TStatus::EDownloading || m_lastStatus == TStatus::ENotDownloaded,
              ("It's only possible to move from {Downloading|NotDownloaded} to Downloading,"
               "old status:",
               m_lastStatus, ", new status:", status));
        break;
      case TStatus::EOnDisk:
        CHECK(m_lastStatus == TStatus::EDownloading || m_lastStatus == TStatus::ENotDownloaded,
              ("It's only possible to move from {Downloading|NotDownloaded} to OnDisk,",
               "old status:", m_lastStatus, ", new status:", status));
        CHECK_EQUAL(m_totalBytesToDownload, m_bytesDownloaded, ());
        break;
      default:
        CHECK(false, ("Unknown state change: from", m_lastStatus, "to", status));
    }

    m_lastStatus = status;
  }

  void OnCountryDownloadingProgress(TIndex const & index, LocalAndRemoteSizeT const & progress)
  {
    if (index != m_index)
      return;
    LOG(LINFO, ("OnCountryDownloadingProgress:", progress));
    CHECK_GREATER(progress.first, m_bytesDownloaded, ());
    m_bytesDownloaded = progress.first;
    m_totalBytesToDownload = progress.second;
    CHECK_LESS_OR_EQUAL(m_bytesDownloaded, m_totalBytesToDownload, ());
  }

  Storage & m_storage;
  TIndex const m_index;
  TStatus m_lastStatus;
  int m_slot;

  int64_t m_bytesDownloaded;
  int64_t m_totalBytesToDownload;
};

void OnCountryDownloaded(string const & mapFileName, TMapOptions files)
{
  LOG(LINFO, ("OnCountryDownloaded:", mapFileName, static_cast<int>(files)));
  Platform & platform = GetPlatform();
  CHECK(my::RenameFileX(
            my::JoinFoldersToPath(platform.WritableDir(), mapFileName + READY_FILE_EXTENSION),
            my::JoinFoldersToPath(platform.WritableDir(), mapFileName)),
        ());
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

UNIT_TEST(StorageTest_Download)
{
  Platform & platform = GetPlatform();

  string const countryFileName = "Azerbaijan";
  string const localFileName =
      my::JoinFoldersToPath(platform.WritableDir(), countryFileName + DATA_FILE_EXTENSION);
  string const downloadedFileName = my::JoinFoldersToPath(
      platform.WritableDir(), countryFileName + DATA_FILE_EXTENSION + READY_FILE_EXTENSION);

  TEST(!Platform::IsFileExistsByFullPath(localFileName),
       ("Please, remove", localFileName, "before running the test."));
  TEST(!Platform::IsFileExistsByFullPath(downloadedFileName),
       ("Please, remove", downloadedFileName, "before running the test."));
  MY_SCOPE_GUARD(removeLocalFile, bind(&FileWriter::DeleteFileX, localFileName));
  MY_SCOPE_GUARD(removeDownloadedFile, bind(&FileWriter::DeleteFileX, downloadedFileName));

  Storage storage;
  storage.Init(&OnCountryDownloaded);

  MessageLoop loop;
  storage.SetDownloaderForTesting(make_unique<FakeMapFilesDownloader>(loop));

  {
    CountryDownloaderChecker checker(storage, countryFileName);
    loop.Run();
  }
}
