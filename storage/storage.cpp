#include "storage/storage.hpp"
#include "storage/http_map_files_downloader.hpp"

#include "defines.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/servers_list.hpp"
#include "platform/settings.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"
#include "coding/url_encode.hpp"

#include "base/gmtime.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/chrono.hpp"
#include "std/sstream.hpp"
#include "std/target_os.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

using namespace downloader;
using namespace platform;

namespace storage
{
namespace
{
uint64_t GetLocalSize(shared_ptr<LocalCountryFile> file, MapOptions opt)
{
  if (!file)
    return 0;
  uint64_t size = 0;
  for (MapOptions bit : {MapOptions::Map, MapOptions::CarRouting})
  {
    if (HasOptions(opt, bit))
      size += file->GetSize(bit);
  }
  return size;
}

uint64_t GetRemoteSize(CountryFile const & file, MapOptions opt, int64_t version)
{
  if (version::IsSingleMwm(version))
    return opt == MapOptions::Nothing ? 0 : file.GetRemoteSize(MapOptions::Map);

  uint64_t size = 0;
  for (MapOptions bit : {MapOptions::Map, MapOptions::CarRouting})
  {
    if (HasOptions(opt, bit))
      size += file.GetRemoteSize(bit);
  }
  return size;
}

void DeleteCountryIndexes(LocalCountryFile const & localFile)
{
  platform::CountryIndexes::DeleteFromDisk(localFile);
}

void DeleteFromDiskWithIndexes(LocalCountryFile const & localFile, MapOptions options)
{
  DeleteCountryIndexes(localFile);
  localFile.DeleteFromDisk(options);
}

TCountryTreeNode const & LeafNodeFromCountryId(TCountryTree const & root,
                                               TCountryId const & countryId)
{
  TCountryTreeNode const * node = root.FindFirstLeaf(countryId);
  CHECK(node, ("Node with id =", countryId, "not found in country tree as a leaf."));
  return *node;
}
}  // namespace

void GetQueuedCountries(Storage::TQueue const & queue, TCountriesSet & resultCountries)
{
  for (auto const & country : queue)
    resultCountries.insert(country.GetCountryId());
}

MapFilesDownloader::TProgress Storage::GetOverallProgress(TCountriesVec const & countries) const
{
  MapFilesDownloader::TProgress overallProgress = {0, 0};
  for (auto const & country : countries)
  {
    NodeAttrs attr;
    GetNodeAttrs(country, attr);

    ASSERT_EQUAL(attr.m_mwmCounter, 1, ());

    if (attr.m_downloadingProgress.second != -1)
    {
      overallProgress.first += attr.m_downloadingProgress.first;
      overallProgress.second += attr.m_downloadingProgress.second;
    }
  }
  return overallProgress;
}

Storage::Storage(string const & pathToCountriesFile /* = COUNTRIES_FILE */,
                 string const & dataDir /* = string() */)
  : m_downloader(new HttpMapFilesDownloader())
  , m_currentSlotId(0)
  , m_dataDir(dataDir)
  , m_downloadMapOnTheMap(nullptr)
{
  SetLocale(languages::GetCurrentTwine());
  LoadCountriesFile(pathToCountriesFile, m_dataDir);
}

Storage::Storage(string const & referenceCountriesTxtJsonForTesting,
                 unique_ptr<MapFilesDownloader> mapDownloaderForTesting)
  : m_downloader(move(mapDownloaderForTesting)), m_currentSlotId(0), m_downloadMapOnTheMap(nullptr)
{
  m_currentVersion =
      LoadCountries(referenceCountriesTxtJsonForTesting, m_countries, m_affiliations);
  CHECK_LESS_OR_EQUAL(0, m_currentVersion, ("Can't load test countries file"));
}

void Storage::Init(TUpdateCallback const & didDownload, TDeleteCallback const & willDelete)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  m_didDownload = didDownload;
  m_willDelete = willDelete;
}

void Storage::DeleteAllLocalMaps(TCountriesVec * existedCountries /* = nullptr */)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  for (auto const & localFiles : m_localFiles)
  {
    for (auto const & localFile : localFiles.second)
    {
      LOG_SHORT(LINFO, ("Remove:", localFiles.first, DebugPrint(*localFile)));
      if (existedCountries)
        existedCountries->push_back(localFiles.first);
      localFile->SyncWithDisk();
      DeleteFromDiskWithIndexes(*localFile, MapOptions::MapWithCarRouting);
    }
  }
}

bool Storage::HaveDownloadedCountries() const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  return !m_localFiles.empty();
}

Storage * Storage::GetPrefetchStorage()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(m_prefetchStorage.get() != nullptr, ());

  return m_prefetchStorage.get();
}

void Storage::PrefetchMigrateData()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  m_prefetchStorage.reset(new Storage(COUNTRIES_FILE, "migrate"));
  m_prefetchStorage->EnableKeepDownloadingQueue(false);
  m_prefetchStorage->Init([](TCountryId const &, TLocalFilePtr const) {},
                          [](TCountryId const &, TLocalFilePtr const) { return false; });
  if (!m_downloadingUrlsForTesting.empty())
    m_prefetchStorage->SetDownloadingUrlsForTesting(m_downloadingUrlsForTesting);
}

void Storage::Migrate(TCountriesVec const & existedCountries)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  platform::migrate::SetMigrationFlag();

  Clear();
  m_countries.Clear();

  TMappingOldMwm mapping;
  LoadCountriesFile(COUNTRIES_FILE, m_dataDir, &mapping);

  vector<TCountryId> prefetchedMaps;
  m_prefetchStorage->GetLocalRealMaps(prefetchedMaps);

  // Move prefetched maps into current storage.
  for (auto const & countryId : prefetchedMaps)
  {
    string prefetchedFilename =
        m_prefetchStorage->GetLatestLocalFile(countryId)->GetPath(MapOptions::Map);
    CountryFile const countryFile = GetCountryFile(countryId);
    auto localFile = PreparePlaceForCountryFiles(GetCurrentDataVersion(), m_dataDir, countryFile);
    string localFilename = localFile->GetPath(MapOptions::Map);
    LOG_SHORT(LINFO, ("Move", prefetchedFilename, "to", localFilename));
    my::RenameFileX(prefetchedFilename, localFilename);
  }

  // Remove empty migrate folder
  Platform::RmDir(m_prefetchStorage->m_dataDir);

  // Cover old big maps with small ones and prepare them to add into download queue
  stringstream ss;
  for (auto const & country : existedCountries)
  {
    ASSERT(!mapping[country].empty(), ());
    for (auto const & smallCountry : mapping[country])
      ss << (ss.str().empty() ? "" : ";") << smallCountry;
  }
  settings::Set("DownloadQueue", ss.str());
}

void Storage::Clear()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  m_downloader->Reset();
  m_queue.clear();
  m_justDownloaded.clear();
  m_failedCountries.clear();
  m_localFiles.clear();
  m_localFilesForFakeCountries.clear();
  SaveDownloadQueue();
}

void Storage::RegisterAllLocalMaps()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  m_localFiles.clear();
  m_localFilesForFakeCountries.clear();

  vector<LocalCountryFile> localFiles;
  FindAllLocalMapsAndCleanup(GetCurrentDataVersion(), m_dataDir, localFiles);

  auto compareByCountryAndVersion = [](LocalCountryFile const & lhs, LocalCountryFile const & rhs) {
    if (lhs.GetCountryFile() != rhs.GetCountryFile())
      return lhs.GetCountryFile() < rhs.GetCountryFile();
    return lhs.GetVersion() > rhs.GetVersion();
  };

  auto equalByCountry = [](LocalCountryFile const & lhs, LocalCountryFile const & rhs) {
    return lhs.GetCountryFile() == rhs.GetCountryFile();
  };

  sort(localFiles.begin(), localFiles.end(), compareByCountryAndVersion);

  auto i = localFiles.begin();
  while (i != localFiles.end())
  {
    auto j = i + 1;
    while (j != localFiles.end() && equalByCountry(*i, *j))
    {
      LocalCountryFile & localFile = *j;
      LOG(LINFO, ("Removing obsolete", localFile));
      localFile.SyncWithDisk();
      DeleteFromDiskWithIndexes(localFile, MapOptions::MapWithCarRouting);
      ++j;
    }

    LocalCountryFile const & localFile = *i;
    string const & name = localFile.GetCountryName();
    TCountryId countryId = FindCountryIdByFile(name);
    if (IsLeaf(countryId))
      RegisterCountryFiles(countryId, localFile.GetDirectory(), localFile.GetVersion());
    else
      RegisterFakeCountryFiles(localFile);

    LOG(LINFO, ("Found file:", name, "in directory:", localFile.GetDirectory()));

    i = j;
  }
  RestoreDownloadQueue();
}

void Storage::GetLocalMaps(vector<TLocalFilePtr> & maps) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  for (auto const & p : m_localFiles)
    maps.push_back(GetLatestLocalFile(p.first));

  for (auto const & p : m_localFilesForFakeCountries)
    maps.push_back(p.second);

  maps.erase(unique(maps.begin(), maps.end()), maps.end());
}

size_t Storage::GetDownloadedFilesCount() const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  return m_localFiles.size();
}

Country const & Storage::CountryLeafByCountryId(TCountryId const & countryId) const
{
  return LeafNodeFromCountryId(m_countries, countryId).Value();
}

Country const & Storage::CountryByCountryId(TCountryId const & countryId) const
{
  TCountryTreeNode const * node = m_countries.FindFirst(countryId);
  CHECK(node, ("Node with id =", countryId, "not found in country tree."));
  return node->Value();
}

bool Storage::IsLeaf(TCountryId const & countryId) const
{
  if (!IsCountryIdValid(countryId))
    return false;
  TCountryTreeNode const * const node = m_countries.FindFirst(countryId);
  return node != nullptr && node->ChildrenCount() == 0;
}

bool Storage::IsInnerNode(TCountryId const & countryId) const
{
  if (!IsCountryIdValid(countryId))
    return false;
  TCountryTreeNode const * const node = m_countries.FindFirst(countryId);
  return node != nullptr && node->ChildrenCount() != 0;
}

TLocalAndRemoteSize Storage::CountrySizeInBytes(TCountryId const & countryId, MapOptions opt) const
{
  QueuedCountry const * queuedCountry = FindCountryInQueue(countryId);
  TLocalFilePtr localFile = GetLatestLocalFile(countryId);
  CountryFile const & countryFile = GetCountryFile(countryId);
  if (queuedCountry == nullptr)
  {
    return TLocalAndRemoteSize(GetLocalSize(localFile, opt),
                               GetRemoteSize(countryFile, opt, GetCurrentDataVersion()));
  }

  TLocalAndRemoteSize sizes(0, GetRemoteSize(countryFile, opt, GetCurrentDataVersion()));
  if (!m_downloader->IsIdle() && IsCountryFirstInQueue(countryId))
  {
    sizes.first =
        m_downloader->GetDownloadingProgress().first +
        GetRemoteSize(countryFile, queuedCountry->GetDownloadedFiles(), GetCurrentDataVersion());
  }
  return sizes;
}

CountryFile const & Storage::GetCountryFile(TCountryId const & countryId) const
{
  return CountryLeafByCountryId(countryId).GetFile();
}

Storage::TLocalFilePtr Storage::GetLatestLocalFile(CountryFile const & countryFile) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  TCountryId const countryId = FindCountryIdByFile(countryFile.GetName());
  if (IsLeaf(countryId))
  {
    TLocalFilePtr localFile = GetLatestLocalFile(countryId);
    if (localFile)
      return localFile;
  }

  auto const it = m_localFilesForFakeCountries.find(countryFile);
  if (it != m_localFilesForFakeCountries.end())
    return it->second;

  return TLocalFilePtr();
}

Storage::TLocalFilePtr Storage::GetLatestLocalFile(TCountryId const & countryId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  auto const it = m_localFiles.find(countryId);
  if (it == m_localFiles.end() || it->second.empty())
    return TLocalFilePtr();

  list<TLocalFilePtr> const & files = it->second;
  TLocalFilePtr latest = files.front();
  for (TLocalFilePtr const & file : files)
  {
    if (file->GetVersion() > latest->GetVersion())
      latest = file;
  }
  return latest;
}

Status Storage::CountryStatus(TCountryId const & countryId) const
{
  // Check if this country has failed while downloading.
  if (m_failedCountries.count(countryId) > 0)
    return Status::EDownloadFailed;

  // Check if we already downloading this country or have it in the queue
  if (IsCountryInQueue(countryId))
  {
    if (IsCountryFirstInQueue(countryId))
      return Status::EDownloading;
    else
      return Status::EInQueue;
  }

  return Status::EUnknown;
}

Status Storage::CountryStatusEx(TCountryId const & countryId) const
{
  return CountryStatusFull(countryId, CountryStatus(countryId));
}

void Storage::CountryStatusEx(TCountryId const & countryId, Status & status,
                              MapOptions & options) const
{
  status = CountryStatusEx(countryId);

  if (status == Status::EOnDisk || status == Status::EOnDiskOutOfDate)
  {
    options = MapOptions::Map;

    TLocalFilePtr localFile = GetLatestLocalFile(countryId);
    ASSERT(localFile, ("Invariant violation: local file out of sync with disk."));
    if (localFile->OnDisk(MapOptions::CarRouting))
      options = SetOptions(options, MapOptions::CarRouting);
  }
}

void Storage::SaveDownloadQueue()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  if (!m_keepDownloadingQueue)
    return;

  stringstream ss;
  for (auto const & item : m_queue)
    ss << (ss.str().empty() ? "" : ";") << item.GetCountryId();
  settings::Set("DownloadQueue", ss.str());
}

void Storage::RestoreDownloadQueue()
{
  if (!m_keepDownloadingQueue)
    return;

  string queue;
  if (!settings::Get("DownloadQueue", queue))
    return;

  strings::SimpleTokenizer iter(queue, ";");
  while (iter)
  {
    DownloadNode(*iter);
    ++iter;
  }
}

void Storage::DownloadCountry(TCountryId const & countryId, MapOptions opt)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  if (opt == MapOptions::Nothing)
    return;

  if (QueuedCountry * queuedCountry = FindCountryInQueue(countryId))
  {
    queuedCountry->AddOptions(opt);
    return;
  }

  m_failedCountries.erase(countryId);
  m_queue.push_back(QueuedCountry(countryId, opt));
  if (m_queue.size() == 1)
    DownloadNextCountryFromQueue();
  else
    NotifyStatusChangedForHierarchy(countryId);
  SaveDownloadQueue();
}

void Storage::DeleteCountry(TCountryId const & countryId, MapOptions opt)
{
  ASSERT(m_willDelete != nullptr, ("Storage::Init wasn't called"));

  TLocalFilePtr localFile = GetLatestLocalFile(countryId);
  opt = NormalizeDeleteFileSet(opt);
  bool const deferredDelete = m_willDelete(countryId, localFile);
  DeleteCountryFiles(countryId, opt, deferredDelete);
  DeleteCountryFilesFromDownloader(countryId, opt);
  NotifyStatusChangedForHierarchy(countryId);
}

void Storage::DeleteCustomCountryVersion(LocalCountryFile const & localFile)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  CountryFile const countryFile = localFile.GetCountryFile();
  DeleteFromDiskWithIndexes(localFile, MapOptions::MapWithCarRouting);

  {
    auto it = m_localFilesForFakeCountries.find(countryFile);
    if (it != m_localFilesForFakeCountries.end())
    {
      m_localFilesForFakeCountries.erase(it);
      return;
    }
  }

  TCountryId const countryId = FindCountryIdByFile(countryFile.GetName());
  if (!(IsLeaf(countryId)))
  {
    LOG(LERROR, ("Removed files for an unknown country:", localFile));
    return;
  }
}

void Storage::NotifyStatusChanged(TCountryId const & countryId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  for (CountryObservers const & observer : m_observers)
    observer.m_changeCountryFn(countryId);
}

void Storage::NotifyStatusChangedForHierarchy(TCountryId const & countryId)
{
  // Notification status changing for a leaf in country tree.
  NotifyStatusChanged(countryId);

  // Notification status changing for ancestors in country tree.
  ForEachAncestorExceptForTheRoot(countryId,
                                  [&](TCountryId const & parentId, TCountryTreeNode const &) {
                                    NotifyStatusChanged(parentId);
                                  });
}

void Storage::DownloadNextCountryFromQueue()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  bool const stopDownload = !m_downloadingPolicy->IsDownloadingAllowed();

  if (m_queue.empty())
  {
    m_downloadingPolicy->ScheduleRetry(m_failedCountries, [this](TCountriesSet const & needReload) {
      for (auto const & country : needReload)
      {
        NodeStatuses status;
        GetNodeStatuses(country, status);
        if (status.m_error == NodeErrorCode::NoInetConnection)
          RetryDownloadNode(country);
      }
    });
    TCountriesVec localMaps;
    GetLocalRealMaps(localMaps);
    GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kMapListing, localMaps);
    if (!localMaps.empty())
      GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kMapDownloadDiscovered);
    return;
  }

  QueuedCountry & queuedCountry = m_queue.front();
  TCountryId const & countryId = queuedCountry.GetCountryId();

  // It's not even possible to prepare directory for files before
  // downloading.  Mark this country as failed and switch to next
  // country.
  if (stopDownload ||
      !PreparePlaceForCountryFiles(GetCurrentDataVersion(), m_dataDir, GetCountryFile(countryId)))
  {
    OnMapDownloadFinished(countryId, false /* success */, queuedCountry.GetInitOptions());
    NotifyStatusChangedForHierarchy(countryId);
    CorrectJustDownloadedAndQueue(m_queue.begin());
    DownloadNextCountryFromQueue();
    return;
  }

  DownloadNextFile(queuedCountry);

  // New status for the country, "Downloading"
  NotifyStatusChangedForHierarchy(queuedCountry.GetCountryId());
}

void Storage::DownloadNextFile(QueuedCountry const & country)
{
  TCountryId const & countryId = country.GetCountryId();
  CountryFile const & countryFile = GetCountryFile(countryId);

  string const filePath = GetFileDownloadPath(countryId, country.GetCurrentFile());
  uint64_t size;

  // It may happen that the file already was downloaded, so there're
  // no need to request servers list and download file.  Let's
  // switch to next file.
  if (GetPlatform().GetFileSizeByFullPath(filePath, size))
  {
    OnMapFileDownloadFinished(true /* success */, MapFilesDownloader::TProgress(size, size));
    return;
  }

  // send Country name for statistics
  m_downloader->GetServersList(GetCurrentDataVersion(), countryFile.GetName(),
                               bind(&Storage::OnServerListDownloaded, this, _1));
}

void Storage::DeleteFromDownloader(TCountryId const & countryId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  DeleteCountryFilesFromDownloader(countryId, MapOptions::MapWithCarRouting);
  NotifyStatusChangedForHierarchy(countryId);
}

bool Storage::IsDownloadInProgress() const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  return !m_queue.empty();
}

TCountryId Storage::GetCurrentDownloadingCountryId() const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  return IsDownloadInProgress() ? m_queue.front().GetCountryId() : storage::TCountryId();
}

void Storage::LoadCountriesFile(string const & pathToCountriesFile, string const & dataDir,
                                TMappingOldMwm * mapping /* = nullptr */)
{
  m_dataDir = dataDir;

  if (!m_dataDir.empty())
  {
    Platform & platform = GetPlatform();
    platform.MkDir(my::JoinFoldersToPath(platform.WritableDir(), m_dataDir));
  }

  if (m_countries.IsEmpty())
  {
    string json;
    ReaderPtr<Reader>(GetPlatform().GetReader(pathToCountriesFile)).ReadAsString(json);
    m_currentVersion = LoadCountries(json, m_countries, m_affiliations, mapping);
    LOG_SHORT(LINFO, ("Loaded countries list for version:", m_currentVersion));
    if (m_currentVersion < 0)
      LOG(LERROR, ("Can't load countries file", pathToCountriesFile));
  }
}

int Storage::Subscribe(TChangeCountryFunction const & change, TProgressFunction const & progress)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  CountryObservers obs;

  obs.m_changeCountryFn = change;
  obs.m_progressFn = progress;
  obs.m_slotId = ++m_currentSlotId;

  m_observers.push_back(obs);

  return obs.m_slotId;
}

void Storage::Unsubscribe(int slotId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  for (auto i = m_observers.begin(); i != m_observers.end(); ++i)
  {
    if (i->m_slotId == slotId)
    {
      m_observers.erase(i);
      return;
    }
  }
}

void Storage::OnMapFileDownloadFinished(bool success,
                                        MapFilesDownloader::TProgress const & progress)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  if (m_queue.empty())
    return;

  QueuedCountry & queuedCountry = m_queue.front();
  TCountryId const countryId = queuedCountry.GetCountryId();

  if (success && queuedCountry.SwitchToNextFile())
  {
    DownloadNextFile(queuedCountry);
    return;
  }

  OnMapDownloadFinished(countryId, success, queuedCountry.GetInitOptions());

  // Send stastics to Push Woosh.
  if (success)
  {
    GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kMapLastDownloaded, countryId);
    char nowStr[18]{};
    auto const tp = std::chrono::system_clock::from_time_t(time(nullptr));
    tm now = my::GmTime(std::chrono::system_clock::to_time_t(tp));
    strftime(nowStr, sizeof(nowStr), "%Y-%m-%d %H:%M", &now);
    GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kMapLastDownloadedTimestamp,
                                                         std::string(nowStr));
  }

  CorrectJustDownloadedAndQueue(m_queue.begin());
  SaveDownloadQueue();

  m_downloader->Reset();
  NotifyStatusChangedForHierarchy(countryId);
  DownloadNextCountryFromQueue();
}

void Storage::ReportProgress(TCountryId const & countryId, MapFilesDownloader::TProgress const & p)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());
  for (CountryObservers const & o : m_observers)
    o.m_progressFn(countryId, p);
}

void Storage::ReportProgressForHierarchy(TCountryId const & countryId,
                                         MapFilesDownloader::TProgress const & leafProgress)
{
  // Reporting progress for a leaf in country tree.
  ReportProgress(countryId, leafProgress);

  // Reporting progress for the parents of the leaf with |countryId|.
  TCountriesSet setQueue;
  GetQueuedCountries(m_queue, setQueue);

  auto calcProgress = [&](TCountryId const & parentId, TCountryTreeNode const & parentNode) {
    TCountriesVec descendants;
    parentNode.ForEachDescendant([&descendants](TCountryTreeNode const & container) {
      descendants.push_back(container.Value().Name());
    });

    MapFilesDownloader::TProgress localAndRemoteBytes =
        CalculateProgress(countryId, descendants, leafProgress, setQueue);
    ReportProgress(parentId, localAndRemoteBytes);
  };

  ForEachAncestorExceptForTheRoot(countryId, calcProgress);
}

void Storage::OnServerListDownloaded(vector<string> const & urls)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  // Queue can be empty because countries were deleted from queue.
  if (m_queue.empty())
    return;

  QueuedCountry const & queuedCountry = m_queue.front();
  TCountryId const & countryId = queuedCountry.GetCountryId();
  MapOptions const file = queuedCountry.GetCurrentFile();

  vector<string> const & downloadingUrls =
      m_downloadingUrlsForTesting.empty() ? urls : m_downloadingUrlsForTesting;
  vector<string> fileUrls;
  fileUrls.reserve(downloadingUrls.size());
  for (string const & url : downloadingUrls)
    fileUrls.push_back(GetFileDownloadUrl(url, countryId, file));

  string const filePath = GetFileDownloadPath(countryId, file);
  m_downloader->DownloadMapFile(fileUrls, filePath, GetDownloadSize(queuedCountry),
                                bind(&Storage::OnMapFileDownloadFinished, this, _1, _2),
                                bind(&Storage::OnMapFileDownloadProgress, this, _1));
}

void Storage::OnMapFileDownloadProgress(MapFilesDownloader::TProgress const & progress)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  // Queue can be empty because countries were deleted from queue.
  if (m_queue.empty())
    return;

  if (m_observers.empty())
    return;

  ReportProgressForHierarchy(m_queue.front().GetCountryId(), progress);
}

bool Storage::RegisterDownloadedFiles(TCountryId const & countryId, MapOptions files)
{
  CountryFile const countryFile = GetCountryFile(countryId);
  TLocalFilePtr localFile = GetLocalFile(countryId, GetCurrentDataVersion());
  if (!localFile)
    localFile = PreparePlaceForCountryFiles(GetCurrentDataVersion(), m_dataDir, countryFile);
  if (!localFile)
  {
    LOG(LERROR, ("Local file data structure can't be prepared for downloaded file(", countryFile,
                 files, ")."));
    return false;
  }

  bool ok = true;
  vector<MapOptions> mapOpt = {MapOptions::Map};
  if (!version::IsSingleMwm(GetCurrentDataVersion()))
    mapOpt.emplace_back(MapOptions::CarRouting);

  for (MapOptions file : mapOpt)
  {
    if (!HasOptions(files, file))
      continue;
    string const path = GetFileDownloadPath(countryId, file);
    if (!my::RenameFileX(path, localFile->GetPath(file)))
    {
      ok = false;
      break;
    }
  }
  localFile->SyncWithDisk();
  if (!ok)
  {
    localFile->DeleteFromDisk(files);
    return false;
  }
  RegisterCountryFiles(localFile);
  return true;
}

void Storage::OnMapDownloadFinished(TCountryId const & countryId, bool success, MapOptions files)
{
  ASSERT(m_didDownload != nullptr, ("Storage::Init wasn't called"));
  ASSERT_NOT_EQUAL(MapOptions::Nothing, files,
                   ("This method should not be called for empty files set."));
  {
    alohalytics::LogEvent(
        "$OnMapDownloadFinished",
        alohalytics::TStringMap({{"name", countryId},
                                 {"status", success ? "ok" : "failed"},
                                 {"version", strings::to_string(GetCurrentDataVersion())},
                                 {"option", DebugPrint(files)}}));
    GetPlatform().GetMarketingService().SendMarketingEvent(marketing::kDownloaderMapActionFinished,
                                                           {{"action", "download"}});
  }

  success = success && RegisterDownloadedFiles(countryId, files);

  if (!success)
  {
    m_failedCountries.insert(countryId);
    return;
  }

  TLocalFilePtr localFile = GetLocalFile(countryId, GetCurrentDataVersion());
  ASSERT(localFile, ());
  DeleteCountryIndexes(*localFile);
  m_didDownload(countryId, localFile);
}

string Storage::GetFileDownloadUrl(string const & baseUrl, TCountryId const & countryId,
                                   MapOptions file) const
{
  CountryFile const & countryFile = GetCountryFile(countryId);

  string const fileName = GetFileName(countryFile.GetName(), file, GetCurrentDataVersion());
  return GetFileDownloadUrl(baseUrl, fileName);
}

string Storage::GetFileDownloadUrl(string const & baseUrl, string const & fName) const
{
  return baseUrl + OMIM_OS_NAME "/" + strings::to_string(GetCurrentDataVersion()) + "/" +
         UrlEncode(fName);
}

TCountryId Storage::FindCountryIdByFile(string const & name) const
{
  // @TODO(bykoianko) Probably it's worth to check here if name represent a node in the tree.
  return TCountryId(name);
}

TCountriesVec Storage::FindAllIndexesByFile(TCountryId const & name) const
{
  // @TODO(bykoianko) This method should be rewritten. At list now name and the param of Find
  // have different types: string and TCountryId.
  TCountriesVec result;
  if (m_countries.FindFirst(name))
    result.push_back(name);
  return result;
}

void Storage::GetOutdatedCountries(vector<Country const *> & countries) const
{
  for (auto const & p : m_localFiles)
  {
    TCountryId const & countryId = p.first;
    string const name = GetCountryFile(countryId).GetName();
    TLocalFilePtr file = GetLatestLocalFile(countryId);
    if (file && file->GetVersion() != GetCurrentDataVersion() && name != WORLD_COASTS_FILE_NAME &&
        name != WORLD_COASTS_OBSOLETE_FILE_NAME && name != WORLD_FILE_NAME)
    {
      countries.push_back(&CountryLeafByCountryId(countryId));
    }
  }
}

Status Storage::CountryStatusWithoutFailed(TCountryId const & countryId) const
{
  // First, check if we already downloading this country or have in in the queue.
  if (!IsCountryInQueue(countryId))
    return CountryStatusFull(countryId, Status::EUnknown);
  return IsCountryFirstInQueue(countryId) ? Status::EDownloading : Status::EInQueue;
}

Status Storage::CountryStatusFull(TCountryId const & countryId, Status const status) const
{
  if (status != Status::EUnknown)
    return status;

  TLocalFilePtr localFile = GetLatestLocalFile(countryId);
  if (!localFile || !localFile->OnDisk(MapOptions::Map))
    return Status::ENotDownloaded;

  CountryFile const & countryFile = GetCountryFile(countryId);
  if (GetRemoteSize(countryFile, MapOptions::Map, GetCurrentDataVersion()) == 0)
    return Status::EUnknown;

  if (localFile->GetVersion() != GetCurrentDataVersion())
    return Status::EOnDiskOutOfDate;
  return Status::EOnDisk;
}

// @TODO(bykoianko) This method does nothing and should be removed.
MapOptions Storage::NormalizeDeleteFileSet(MapOptions options) const
{
  // Car routing files are useless without map files.
  if (HasOptions(options, MapOptions::Map))
    options = SetOptions(options, MapOptions::CarRouting);
  return options;
}

QueuedCountry * Storage::FindCountryInQueue(TCountryId const & countryId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  auto it = find(m_queue.begin(), m_queue.end(), countryId);
  return it == m_queue.end() ? nullptr : &*it;
}

QueuedCountry const * Storage::FindCountryInQueue(TCountryId const & countryId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  auto it = find(m_queue.begin(), m_queue.end(), countryId);
  return it == m_queue.end() ? nullptr : &*it;
}

bool Storage::IsCountryInQueue(TCountryId const & countryId) const
{
  return FindCountryInQueue(countryId) != nullptr;
}

bool Storage::IsCountryFirstInQueue(TCountryId const & countryId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  return !m_queue.empty() && m_queue.front().GetCountryId() == countryId;
}

void Storage::SetLocale(string const & locale) { m_countryNameGetter.SetLocale(locale); }
string Storage::GetLocale() const { return m_countryNameGetter.GetLocale(); }
void Storage::SetDownloaderForTesting(unique_ptr<MapFilesDownloader> && downloader)
{
  m_downloader = move(downloader);
}

void Storage::SetCurrentDataVersionForTesting(int64_t currentVersion)
{
  m_currentVersion = currentVersion;
}

void Storage::SetDownloadingUrlsForTesting(vector<string> const & downloadingUrls)
{
  m_downloadingUrlsForTesting = downloadingUrls;
}

void Storage::SetLocaleForTesting(string const & jsonBuffer, string const & locale)
{
  m_countryNameGetter.SetLocaleForTesting(jsonBuffer, locale);
}

Storage::TLocalFilePtr Storage::GetLocalFile(TCountryId const & countryId, int64_t version) const
{
  auto const it = m_localFiles.find(countryId);
  if (it == m_localFiles.end() || it->second.empty())
    return TLocalFilePtr();

  for (auto const & file : it->second)
  {
    if (file->GetVersion() == version)
      return file;
  }
  return TLocalFilePtr();
}

void Storage::RegisterCountryFiles(TLocalFilePtr localFile)
{
  CHECK(localFile, ());
  localFile->SyncWithDisk();

  for (auto const & countryId : FindAllIndexesByFile(localFile->GetCountryName()))
  {
    TLocalFilePtr existingFile = GetLocalFile(countryId, localFile->GetVersion());
    if (existingFile)
      ASSERT_EQUAL(localFile.get(), existingFile.get(), ());
    else
      m_localFiles[countryId].push_front(localFile);
  }
}

void Storage::RegisterCountryFiles(TCountryId const & countryId, string const & directory,
                                   int64_t version)
{
  TLocalFilePtr localFile = GetLocalFile(countryId, version);
  if (localFile)
    return;

  CountryFile const & countryFile = GetCountryFile(countryId);
  localFile = make_shared<LocalCountryFile>(directory, countryFile, version);
  RegisterCountryFiles(localFile);
}

void Storage::RegisterFakeCountryFiles(platform::LocalCountryFile const & localFile)
{
  if (localFile.GetCountryName() ==
      (platform::migrate::NeedMigrate() ? WORLD_COASTS_FILE_NAME : WORLD_COASTS_OBSOLETE_FILE_NAME))
    return;

  TLocalFilePtr fakeCountryLocalFile = make_shared<LocalCountryFile>(localFile);
  fakeCountryLocalFile->SyncWithDisk();
  m_localFilesForFakeCountries[fakeCountryLocalFile->GetCountryFile()] = fakeCountryLocalFile;
}

void Storage::DeleteCountryFiles(TCountryId const & countryId, MapOptions opt, bool deferredDelete)
{
  auto const it = m_localFiles.find(countryId);
  if (it == m_localFiles.end())
    return;

  if (deferredDelete)
  {
    m_localFiles.erase(countryId);
    return;
  }

  auto & localFiles = it->second;
  for (auto & localFile : localFiles)
  {
    DeleteFromDiskWithIndexes(*localFile, opt);
    localFile->SyncWithDisk();
    if (localFile->GetFiles() == MapOptions::Nothing)
      localFile.reset();
  }
  auto isNull = [](TLocalFilePtr const & localFile) { return !localFile; };
  localFiles.remove_if(isNull);
  if (localFiles.empty())
    m_localFiles.erase(countryId);
}

bool Storage::DeleteCountryFilesFromDownloader(TCountryId const & countryId, MapOptions opt)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  QueuedCountry * queuedCountry = FindCountryInQueue(countryId);
  if (!queuedCountry)
    return false;

  opt = IntersectOptions(opt, queuedCountry->GetInitOptions());

  if (IsCountryFirstInQueue(countryId))
  {
    // Abrupt downloading of the current file if it should be removed.
    if (HasOptions(opt, queuedCountry->GetCurrentFile()))
      m_downloader->Reset();

    // Remove all files downloader had been created for a country.
    DeleteDownloaderFilesForCountry(GetCurrentDataVersion(), m_dataDir, GetCountryFile(countryId));
  }

  queuedCountry->RemoveOptions(opt);

  // Remove country from the queue if there's nothing to download.
  if (queuedCountry->GetInitOptions() == MapOptions::Nothing)
  {
    auto it = find(m_queue.cbegin(), m_queue.cend(), countryId);
    ASSERT(it != m_queue.cend(), ());
    if (m_queue.size() == 1)
    {  // If m_queue is about to be empty.
      m_justDownloaded.clear();
      m_queue.clear();
    }
    else
    {  // A deleted map should not be moved to m_justDownloaded.
      m_queue.erase(it);
    }
    SaveDownloadQueue();
  }

  if (!m_queue.empty() && m_downloader->IsIdle())
  {
    // Kick possibly interrupted downloader.
    if (IsCountryFirstInQueue(countryId))
      DownloadNextFile(m_queue.front());
    else
      DownloadNextCountryFromQueue();
  }
  return true;
}

uint64_t Storage::GetDownloadSize(QueuedCountry const & queuedCountry) const
{
  CountryFile const & file = GetCountryFile(queuedCountry.GetCountryId());
  return GetRemoteSize(file, queuedCountry.GetCurrentFile(), GetCurrentDataVersion());
}

string Storage::GetFileDownloadPath(TCountryId const & countryId, MapOptions file) const
{
  return platform::GetFileDownloadPath(GetCurrentDataVersion(), m_dataDir,
                                       GetCountryFile(countryId), file);
}

bool Storage::CheckFailedCountries(TCountriesVec const & countries) const
{
  for (auto const & country : countries)
  {
    if (m_failedCountries.count(country))
      return true;
  }
  return false;
}

TCountryId const Storage::GetRootId() const { return m_countries.GetRoot().Value().Name(); }
void Storage::GetChildren(TCountryId const & parent, TCountriesVec & childIds) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  TCountryTreeNode const * const parentNode = m_countries.FindFirst(parent);
  if (parentNode == nullptr)
  {
    ASSERT(false, ("TCountryId =", parent, "not found in m_countries."));
    return;
  }

  size_t const childrenCount = parentNode->ChildrenCount();
  childIds.clear();
  childIds.reserve(childrenCount);
  for (size_t i = 0; i < childrenCount; ++i)
    childIds.emplace_back(parentNode->Child(i).Value().Name());
}

void Storage::GetLocalRealMaps(TCountriesVec & localMaps) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  localMaps.clear();
  localMaps.reserve(m_localFiles.size());

  for (auto const & keyValue : m_localFiles)
    localMaps.push_back(keyValue.first);
}

void Storage::GetChildrenInGroups(TCountryId const & parent, TCountriesVec & downloadedChildren,
                                  TCountriesVec & availChildren, bool keepAvailableChildren) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  TCountryTreeNode const * const parentNode = m_countries.FindFirst(parent);
  if (parentNode == nullptr)
  {
    ASSERT(false, ("TCountryId =", parent, "not found in m_countries."));
    return;
  }

  downloadedChildren.clear();
  availChildren.clear();

  // Vector of disputed territories which are downloaded but the other maps
  // in their group are not downloaded. Only mwm in subtree with root == |parent|
  // are taken into account.
  TCountriesVec disputedTerritoriesWithoutSiblings;
  // All disputed territories in subtree with root == |parent|.
  TCountriesVec allDisputedTerritories;
  parentNode->ForEachChild([&](TCountryTreeNode const & childNode) {
    vector<pair<TCountryId, NodeStatus>> disputedTerritoriesAndStatus;
    StatusAndError const childStatus = GetNodeStatusInfo(childNode,
                                                         disputedTerritoriesAndStatus,
                                                         true /* isDisputedTerritoriesCounted */);

    TCountryId const & childValue = childNode.Value().Name();
    ASSERT_NOT_EQUAL(childStatus.status, NodeStatus::Undefined, ());
    for (auto const & disputed : disputedTerritoriesAndStatus)
      allDisputedTerritories.push_back(disputed.first);

    if (childStatus.status == NodeStatus::NotDownloaded)
    {
      availChildren.push_back(childValue);
      for (auto const & disputed : disputedTerritoriesAndStatus)
      {
        if (disputed.second != NodeStatus::NotDownloaded)
          disputedTerritoriesWithoutSiblings.push_back(disputed.first);
      }
    }
    else
    {
      downloadedChildren.push_back(childValue);
      if (keepAvailableChildren)
        availChildren.push_back(childValue);
    }
  });

  TCountriesVec uniqueDisputed(disputedTerritoriesWithoutSiblings.begin(),
                               disputedTerritoriesWithoutSiblings.end());
  my::SortUnique(uniqueDisputed);

  for (auto const & countryId : uniqueDisputed)
  {
    // Checks that the number of disputed territories with |countryId| in subtree with root ==
    // |parent|
    // is equal to the number of disputed territories with out downloaded sibling
    // with |countryId| in subtree with root == |parent|.
    if (count(disputedTerritoriesWithoutSiblings.begin(), disputedTerritoriesWithoutSiblings.end(),
              countryId) ==
        count(allDisputedTerritories.begin(), allDisputedTerritories.end(), countryId))
    {
      // |countryId| is downloaded without any other map in its group.
      downloadedChildren.push_back(countryId);
    }
  }
}

bool Storage::IsNodeDownloaded(TCountryId const & countryId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  for (auto const & localeMap : m_localFiles)
  {
    if (countryId == localeMap.first)
      return true;
  }
  return false;
}

bool Storage::HasLatestVersion(TCountryId const & countryId) const
{
  return CountryStatusEx(countryId) == Status::EOnDisk;
}

void Storage::DownloadNode(TCountryId const & countryId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  TCountryTreeNode const * const node = m_countries.FindFirst(countryId);

  if (!node)
    return;

  if (GetNodeStatus(*node).status == NodeStatus::OnDisk)
    return;

  auto downloadAction = [this](TCountryTreeNode const & descendantNode) {
    if (descendantNode.ChildrenCount() == 0 &&
        GetNodeStatus(descendantNode).status != NodeStatus::OnDisk)
      this->DownloadCountry(descendantNode.Value().Name(), MapOptions::MapWithCarRouting);
  };

  node->ForEachInSubtree(downloadAction);
}

void Storage::DeleteNode(TCountryId const & countryId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  TCountryTreeNode const * const node = m_countries.FindFirst(countryId);

  if (!node)
    return;

  auto deleteAction = [this](TCountryTreeNode const & descendantNode) {
    bool onDisk = m_localFiles.find(descendantNode.Value().Name()) != m_localFiles.end();
    if (descendantNode.ChildrenCount() == 0 && onDisk)
      this->DeleteCountry(descendantNode.Value().Name(), MapOptions::MapWithCarRouting);
  };
  node->ForEachInSubtree(deleteAction);
}

StatusAndError Storage::GetNodeStatus(TCountryTreeNode const & node) const
{
  vector<pair<TCountryId, NodeStatus>> disputedTerritories;
  return GetNodeStatusInfo(node, disputedTerritories, false /* isDisputedTerritoriesCounted */);
}

bool Storage::IsDisputed(TCountryTreeNode const & node) const
{
  vector<TCountryTreeNode const *> found;
  m_countries.Find(node.Value().Name(), found);
  return found.size() > 1;
}

StatusAndError Storage::GetNodeStatusInfo(
    TCountryTreeNode const & node, vector<pair<TCountryId, NodeStatus>> & disputedTerritories,
                                          bool isDisputedTerritoriesCounted) const
{
  // Leaf node status.
  if (node.ChildrenCount() == 0)
  {
    StatusAndError const statusAndError = ParseStatus(CountryStatusEx(node.Value().Name()));
    if (IsDisputed(node))
      disputedTerritories.push_back(make_pair(node.Value().Name(), statusAndError.status));
    return statusAndError;
  }

  // Group node status.
  NodeStatus result = NodeStatus::NotDownloaded;
  bool allOnDisk = true;

  auto groupStatusCalculator = [&](TCountryTreeNode const & nodeInSubtree) {
    StatusAndError const statusAndError =
        ParseStatus(CountryStatusEx(nodeInSubtree.Value().Name()));

    if (IsDisputed(nodeInSubtree) && isDisputedTerritoriesCounted)
    {
      disputedTerritories.push_back(make_pair(nodeInSubtree.Value().Name(), statusAndError.status));
      return;
    }

    if (result == NodeStatus::Downloading || nodeInSubtree.ChildrenCount() != 0)
      return;

    if (statusAndError.status != NodeStatus::OnDisk)
      allOnDisk = false;

    if (static_cast<size_t>(statusAndError.status) < static_cast<size_t>(result))
      result = statusAndError.status;
  };

  node.ForEachDescendant(groupStatusCalculator);
  if (allOnDisk)
    return ParseStatus(Status::EOnDisk);
  if (result == NodeStatus::OnDisk)
    return {NodeStatus::Partly, NodeErrorCode::NoError};

  ASSERT_NOT_EQUAL(result, NodeStatus::Undefined, ());
  return {result, NodeErrorCode::NoError};
}

void Storage::GetNodeAttrs(TCountryId const & countryId, NodeAttrs & nodeAttrs) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  vector<TCountryTreeNode const *> nodes;
  m_countries.Find(countryId, nodes);
  CHECK(!nodes.empty(), (countryId));
  // If nodes.size() > 1 countryId corresponds to a disputed territories.
  // In that case it's guaranteed that most of attributes are equal for
  // each element of nodes. See Country class description for further details.
  TCountryTreeNode const * const node = nodes[0];

  Country const & nodeValue = node->Value();
  nodeAttrs.m_mwmCounter = nodeValue.GetSubtreeMwmCounter();
  nodeAttrs.m_mwmSize = nodeValue.GetSubtreeMwmSizeBytes();
  StatusAndError statusAndErr = GetNodeStatus(*node);
  nodeAttrs.m_status = statusAndErr.status;
  nodeAttrs.m_error = statusAndErr.error;
  nodeAttrs.m_nodeLocalName = m_countryNameGetter(countryId);
  nodeAttrs.m_nodeLocalDescription =
      m_countryNameGetter.Get(countryId + LOCALIZATION_DESCRIPTION_SUFFIX);

  // Progress.
  if (nodeAttrs.m_status == NodeStatus::OnDisk)
  {
    // Group or leaf node is on disk and up to date.
    TMwmSize const subTreeSizeBytes = node->Value().GetSubtreeMwmSizeBytes();
    nodeAttrs.m_downloadingProgress.first = subTreeSizeBytes;
    nodeAttrs.m_downloadingProgress.second = subTreeSizeBytes;
  }
  else
  {
    TCountriesVec subtree;
    node->ForEachInSubtree(
        [&subtree](TCountryTreeNode const & d) { subtree.push_back(d.Value().Name()); });
    TCountryId const & downloadingMwm =
        IsDownloadInProgress() ? GetCurrentDownloadingCountryId() : kInvalidCountryId;
    MapFilesDownloader::TProgress downloadingMwmProgress(0, 0);
    if (!m_downloader->IsIdle())
    {
      downloadingMwmProgress = m_downloader->GetDownloadingProgress();
      // If we don't know estimated file size then we ignore its progress.
      if (downloadingMwmProgress.second == -1)
        downloadingMwmProgress = {0, 0};
    }

    TCountriesSet setQueue;
    GetQueuedCountries(m_queue, setQueue);
    nodeAttrs.m_downloadingProgress =
        CalculateProgress(downloadingMwm, subtree, downloadingMwmProgress, setQueue);
  }

  // Local mwm information and information about downloading mwms.
  nodeAttrs.m_localMwmCounter = 0;
  nodeAttrs.m_localMwmSize = 0;
  nodeAttrs.m_downloadingMwmCounter = 0;
  nodeAttrs.m_downloadingMwmSize = 0;
  TCountriesSet visitedLocalNodes;
  node->ForEachInSubtree([this, &nodeAttrs, &visitedLocalNodes](TCountryTreeNode const & d) {
    TCountryId const countryId = d.Value().Name();
    if (visitedLocalNodes.count(countryId) != 0)
      return;
    visitedLocalNodes.insert(countryId);

    // Downloading mwm information.
    StatusAndError const statusAndErr = GetNodeStatus(d);
    ASSERT_NOT_EQUAL(statusAndErr.status, NodeStatus::Undefined, ());
    if (statusAndErr.status != NodeStatus::NotDownloaded &&
        statusAndErr.status != NodeStatus::Partly && d.ChildrenCount() == 0)
    {
      nodeAttrs.m_downloadingMwmCounter += 1;
      nodeAttrs.m_downloadingMwmSize += d.Value().GetSubtreeMwmSizeBytes();
    }

    // Local mwm information.
    Storage::TLocalFilePtr const localFile = GetLatestLocalFile(countryId);
    if (localFile == nullptr)
      return;

    nodeAttrs.m_localMwmCounter += 1;
    nodeAttrs.m_localMwmSize += localFile->GetSize(MapOptions::Map);
  });
  nodeAttrs.m_present = m_localFiles.find(countryId) != m_localFiles.end();

  // Parents information.
  nodeAttrs.m_parentInfo.clear();
  nodeAttrs.m_parentInfo.reserve(nodes.size());
  for (auto const & n : nodes)
  {
    Country const & nValue = n->Value();
    CountryIdAndName countryIdAndName;
    countryIdAndName.m_id = nValue.GetParent();
    if (countryIdAndName.m_id.empty())  // The root case.
      countryIdAndName.m_localName = string();
    else
      countryIdAndName.m_localName = m_countryNameGetter(countryIdAndName.m_id);
    nodeAttrs.m_parentInfo.emplace_back(move(countryIdAndName));
  }
  // Parents country.
  nodeAttrs.m_topmostParentInfo.clear();
  ForEachAncestorExceptForTheRoot(
      nodes, [&](TCountryId const & ancestorId, TCountryTreeNode const & node) {
        if (node.Value().GetParent() == GetRootId())
          nodeAttrs.m_topmostParentInfo.push_back({ancestorId, m_countryNameGetter(ancestorId)});
      });
}

void Storage::GetNodeStatuses(TCountryId const & countryId, NodeStatuses & nodeStatuses) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  TCountryTreeNode const * const node = m_countries.FindFirst(countryId);
  CHECK(node, (countryId));

  StatusAndError statusAndErr = GetNodeStatus(*node);
  nodeStatuses.m_status = statusAndErr.status;
  nodeStatuses.m_error = statusAndErr.error;
  nodeStatuses.m_groupNode = (node->ChildrenCount() != 0);
}

void Storage::SetCallbackForClickOnDownloadMap(TDownloadFn & downloadFn)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  m_downloadMapOnTheMap = downloadFn;
}

void Storage::DoClickOnDownloadMap(TCountryId const & countryId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  if (m_downloadMapOnTheMap)
    m_downloadMapOnTheMap(countryId);
}

MapFilesDownloader::TProgress Storage::CalculateProgress(
    TCountryId const & downloadingMwm, TCountriesVec const & mwms,
    MapFilesDownloader::TProgress const & downloadingMwmProgress,
    TCountriesSet const & mwmsInQueue) const
{
  // Function calculates progress correctly OLNY if |downloadingMwm| is leaf.

  MapFilesDownloader::TProgress localAndRemoteBytes = make_pair(0, 0);

  for (auto const & d : mwms)
  {
    if (downloadingMwm == d && downloadingMwm != kInvalidCountryId)
    {
      localAndRemoteBytes.first += downloadingMwmProgress.first;
      localAndRemoteBytes.second += GetCountryFile(d).GetRemoteSize(MapOptions::Map);
    }
    else if (mwmsInQueue.count(d) != 0)
    {
      localAndRemoteBytes.second += GetCountryFile(d).GetRemoteSize(MapOptions::Map);
    }
    else if (m_justDownloaded.count(d) != 0)
    {
      TMwmSize const localCountryFileSz = GetCountryFile(d).GetRemoteSize(MapOptions::Map);
      localAndRemoteBytes.first += localCountryFileSz;
      localAndRemoteBytes.second += localCountryFileSz;
    }
  }

  return localAndRemoteBytes;
}

void Storage::UpdateNode(TCountryId const & countryId)
{
  ForEachInSubtree(countryId, [this](TCountryId const & descendantId, bool groupNode) {
    if (!groupNode && m_localFiles.find(descendantId) != m_localFiles.end())
      this->DownloadNode(descendantId);
  });
}

void Storage::CancelDownloadNode(TCountryId const & countryId)
{
  TCountriesSet setQueue;
  GetQueuedCountries(m_queue, setQueue);

  ForEachInSubtree(countryId, [&](TCountryId const & descendantId, bool /* groupNode */) {
    if (setQueue.count(descendantId) != 0)
      DeleteFromDownloader(descendantId);
    if (m_failedCountries.count(descendantId) != 0)
    {
      m_failedCountries.erase(descendantId);
      NotifyStatusChangedForHierarchy(countryId);
    }
  });
}

void Storage::RetryDownloadNode(TCountryId const & countryId)
{
  ForEachInSubtree(countryId, [this](TCountryId const & descendantId, bool groupNode) {
    if (!groupNode && m_failedCountries.count(descendantId) != 0)
      DownloadNode(descendantId);
  });
}

bool Storage::GetUpdateInfo(TCountryId const & countryId, UpdateInfo & updateInfo) const
{
  auto const updateInfoAccumulator = [&updateInfo, this](TCountryTreeNode const & descendantNode) {
    if (descendantNode.ChildrenCount() != 0 ||
        GetNodeStatus(descendantNode).status != NodeStatus::OnDiskOutOfDate)
      return;
    updateInfo.m_numberOfMwmFilesToUpdate += 1;  // It's not a group mwm.
    updateInfo.m_totalUpdateSizeInBytes += descendantNode.Value().GetSubtreeMwmSizeBytes();
    TLocalAndRemoteSize sizes =
        CountrySizeInBytes(descendantNode.Value().Name(), MapOptions::MapWithCarRouting);
    updateInfo.m_sizeDifference +=
        static_cast<int64_t>(sizes.second) - static_cast<int64_t>(sizes.first);
  };

  TCountryTreeNode const * const node = m_countries.FindFirst(countryId);
  if (!node)
  {
    ASSERT(false, ());
    return false;
  }
  updateInfo = UpdateInfo();
  node->ForEachInSubtree(updateInfoAccumulator);
  return true;
}

void Storage::CorrectJustDownloadedAndQueue(TQueue::iterator justDownloadedItem)
{
  m_justDownloaded.insert(justDownloadedItem->GetCountryId());
  m_queue.erase(justDownloadedItem);
  if (m_queue.empty())
    m_justDownloaded.clear();
}

void Storage::GetQueuedChildren(TCountryId const & parent, TCountriesVec & queuedChildren) const
{
  TCountryTreeNode const * const node = m_countries.FindFirst(parent);
  if (!node)
  {
    ASSERT(false, ());
    return;
  }

  queuedChildren.clear();
  node->ForEachChild([&queuedChildren, this](TCountryTreeNode const & child) {
    NodeStatus status = GetNodeStatus(child).status;
    ASSERT_NOT_EQUAL(status, NodeStatus::Undefined, ());
    if (status == NodeStatus::Downloading || status == NodeStatus::InQueue)
      queuedChildren.push_back(child.Value().Name());
  });
}

void Storage::GetGroupNodePathToRoot(TCountryId const & groupNode, TCountriesVec & path) const
{
  path.clear();

  vector<TCountryTreeNode const *> nodes;
  m_countries.Find(groupNode, nodes);
  if (nodes.empty())
  {
    LOG(LWARNING, ("TCountryId =", groupNode, "not found in m_countries."));
    return;
  }

  if (nodes.size() != 1)
  {
    LOG(LWARNING, (groupNode, "Group node can't have more than one parent."));
    return;
  }

  if (nodes[0]->ChildrenCount() == 0)
  {
    LOG(LWARNING, (nodes[0]->Value().Name(), "is a leaf node."));
    return;
  }

  ForEachAncestorExceptForTheRoot(
      nodes, [&path](TCountryId const & id, TCountryTreeNode const &) { path.push_back(id); });
  path.push_back(m_countries.GetRoot().Value().Name());
}

void Storage::GetTopmostNodesFor(TCountryId const & countryId, TCountriesVec & nodes,
                                 size_t level) const
{
  nodes.clear();

  vector<TCountryTreeNode const *> treeNodes;
  m_countries.Find(countryId, treeNodes);
  if (treeNodes.empty())
  {
    LOG(LWARNING, ("TCountryId =", countryId, "not found in m_countries."));
    return;
  }

  nodes.resize(treeNodes.size());
  for (size_t i = 0; i < treeNodes.size(); ++i)
  {
    nodes[i] = countryId;
    TCountriesVec path;
    ForEachAncestorExceptForTheRoot(
        {treeNodes[i]},
        [&path](TCountryId const & id, TCountryTreeNode const &) { path.emplace_back(id); });
    if (!path.empty() && level < path.size())
      nodes[i] = path[path.size() - 1 - level];
  }
}

}  // namespace storage
