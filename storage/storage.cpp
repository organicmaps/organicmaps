#include "storage/storage.hpp"
#include "storage/http_map_files_downloader.hpp"

#include "defines.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/marketing_service.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/servers_list.hpp"
#include "platform/settings.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/url_encode.hpp"

#include "base/exception.hpp"
#include "base/gmtime.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <chrono>
#include <sstream>

#include "std/target_os.hpp"

#include <limits>

#include "3party/Alohalytics/src/alohalytics.h"

using namespace downloader;
using namespace generator::mwm_diff;
using namespace platform;
using namespace std::chrono;
using namespace std;

namespace storage
{
namespace
{
string const kUpdateQueueKey = "UpdateQueue";
string const kDownloadQueueKey = "DownloadQueue";

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

void DeleteCountryIndexes(LocalCountryFile const & localFile)
{
  platform::CountryIndexes::DeleteFromDisk(localFile);
}

void DeleteFromDiskWithIndexes(LocalCountryFile const & localFile, MapOptions options)
{
  DeleteCountryIndexes(localFile);
  localFile.DeleteFromDisk(options);
}

CountryTreeNode const & LeafNodeFromCountryId(CountryTree const & root, CountryId const & countryId)
{
  CountryTreeNode const * node = root.FindFirstLeaf(countryId);
  CHECK(node, ("Node with id =", countryId, "not found in country tree as a leaf."));
  return *node;
}

bool ValidateIntegrity(LocalFilePtr mapLocalFile, string const & countryId, string const & source)
{
  int64_t const version = mapLocalFile->GetVersion();

  int64_t constexpr kMinSupportedVersion = 181030;
  if (version < kMinSupportedVersion)
    return true;

  if (mapLocalFile->ValidateIntegrity())
    return true;

  alohalytics::LogEvent("$MapIntegrityFailure",
                        alohalytics::TStringMap({{"mwm", countryId},
                                                 {"version", strings::to_string(version)},
                                                 {"source", source}}));
  return false;
}
}  // namespace

void GetQueuedCountries(Storage::Queue const & queue, CountriesSet & resultCountries)
{
  for (auto const & country : queue)
    resultCountries.insert(country.GetCountryId());
}

MapFilesDownloader::Progress Storage::GetOverallProgress(CountriesVec const & countries) const
{
  MapFilesDownloader::Progress overallProgress = {0, 0};
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
  : m_downloader(make_unique<HttpMapFilesDownloader>())
  , m_currentSlotId(0)
  , m_dataDir(dataDir)
  , m_downloadMapOnTheMap(nullptr)
  , m_maxMwmSizeBytes(0)
{
  SetLocale(languages::GetCurrentTwine());
  LoadCountriesFile(pathToCountriesFile, m_dataDir);
  CalcMaxMwmSizeBytes();
}

Storage::Storage(string const & referenceCountriesTxtJsonForTesting,
                 unique_ptr<MapFilesDownloader> mapDownloaderForTesting)
  : m_downloader(move(mapDownloaderForTesting))
  , m_currentSlotId(0)
  , m_downloadMapOnTheMap(nullptr)
  , m_maxMwmSizeBytes(0)
{
  m_currentVersion =
      LoadCountriesFromBuffer(referenceCountriesTxtJsonForTesting, m_countries, m_affiliations);
  CHECK_LESS_OR_EQUAL(0, m_currentVersion, ("Can't load test countries file"));
  CalcMaxMwmSizeBytes();
}

void Storage::Init(UpdateCallback const & didDownload, DeleteCallback const & willDelete)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_didDownload = didDownload;
  m_willDelete = willDelete;
}

void Storage::DeleteAllLocalMaps(CountriesVec * existedCountries /* = nullptr */)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

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
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  return !m_localFiles.empty();
}

Storage * Storage::GetPrefetchStorage()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(m_prefetchStorage.get() != nullptr, ());

  return m_prefetchStorage.get();
}

void Storage::PrefetchMigrateData()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_prefetchStorage.reset(new Storage(COUNTRIES_FILE, "migrate"));
  m_prefetchStorage->EnableKeepDownloadingQueue(false);
  m_prefetchStorage->Init([](CountryId const &, LocalFilePtr const) {},
                          [](CountryId const &, LocalFilePtr const) { return false; });
  if (!m_downloadingUrlsForTesting.empty())
    m_prefetchStorage->SetDownloadingUrlsForTesting(m_downloadingUrlsForTesting);
}

void Storage::Migrate(CountriesVec const & existedCountries)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  platform::migrate::SetMigrationFlag();

  Clear();
  m_countries.Clear();

  TMappingOldMwm mapping;
  LoadCountriesFile(COUNTRIES_FILE, m_dataDir, &mapping);

  vector<CountryId> prefetchedMaps;
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
    base::RenameFileX(prefetchedFilename, localFilename);
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
  settings::Set(kDownloadQueueKey, ss.str());
}

void Storage::Clear()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_downloader->Reset();
  m_queue.clear();
  m_justDownloaded.clear();
  m_failedCountries.clear();
  m_localFiles.clear();
  m_localFilesForFakeCountries.clear();
  SaveDownloadQueue();
}

void Storage::RegisterAllLocalMaps(bool enableDiffs)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

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

  int64_t minVersion = std::numeric_limits<int64_t>().max();
  int64_t maxVersion = std::numeric_limits<int64_t>().min();

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
    if (name != WORLD_FILE_NAME && name != WORLD_COASTS_FILE_NAME &&
        name != WORLD_COASTS_OBSOLETE_FILE_NAME)
    {
      auto const version = localFile.GetVersion();
      if (version < minVersion)
        minVersion = version;
      if (version > maxVersion)
        maxVersion = version;
    }

    CountryId countryId = FindCountryIdByFile(name);
    if (IsLeaf(countryId))
      RegisterCountryFiles(countryId, localFile.GetDirectory(), localFile.GetVersion());
    else
      RegisterFakeCountryFiles(localFile);

    LOG(LINFO, ("Found file:", name, "in directory:", localFile.GetDirectory()));

    i = j;
  }

  if (minVersion > maxVersion)
  {
    minVersion = 0;
    maxVersion = 0;
  }

  GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kMapVersionMin,
                                                       strings::to_string(minVersion));
  GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kMapVersionMax,
                                                       strings::to_string(maxVersion));

  FindAllDiffs(m_dataDir, m_notAppliedDiffs);
  if (enableDiffs)
    LoadDiffScheme();
  RestoreDownloadQueue();
}

void Storage::GetLocalMaps(vector<LocalFilePtr> & maps) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  for (auto const & p : m_localFiles)
    maps.push_back(GetLatestLocalFile(p.first));

  for (auto const & p : m_localFilesForFakeCountries)
    maps.push_back(p.second);

  maps.erase(unique(maps.begin(), maps.end()), maps.end());
}

size_t Storage::GetDownloadedFilesCount() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  return m_localFiles.size();
}

Country const & Storage::CountryLeafByCountryId(CountryId const & countryId) const
{
  return LeafNodeFromCountryId(m_countries, countryId).Value();
}

Country const & Storage::CountryByCountryId(CountryId const & countryId) const
{
  CountryTreeNode const * node = m_countries.FindFirst(countryId);
  CHECK(node, ("Node with id =", countryId, "not found in country tree."));
  return node->Value();
}

bool Storage::IsLeaf(CountryId const & countryId) const
{
  if (!IsCountryIdValid(countryId))
    return false;
  CountryTreeNode const * const node = m_countries.FindFirst(countryId);
  return node != nullptr && node->ChildrenCount() == 0;
}

bool Storage::IsInnerNode(CountryId const & countryId) const
{
  if (!IsCountryIdValid(countryId))
    return false;
  CountryTreeNode const * const node = m_countries.FindFirst(countryId);
  return node != nullptr && node->ChildrenCount() != 0;
}

LocalAndRemoteSize Storage::CountrySizeInBytes(CountryId const & countryId, MapOptions opt) const
{
  QueuedCountry const * queuedCountry = FindCountryInQueue(countryId);
  LocalFilePtr localFile = GetLatestLocalFile(countryId);
  CountryFile const & countryFile = GetCountryFile(countryId);
  if (queuedCountry == nullptr)
  {
    return LocalAndRemoteSize(GetLocalSize(localFile, opt),
                              GetRemoteSize(countryFile, opt, GetCurrentDataVersion()));
  }

  LocalAndRemoteSize sizes(0, GetRemoteSize(countryFile, opt, GetCurrentDataVersion()));
  if (!m_downloader->IsIdle() && IsCountryFirstInQueue(countryId))
  {
    sizes.first = m_downloader->GetDownloadingProgress().first +
                  GetRemoteSize(countryFile, queuedCountry->GetDownloadedFilesOptions(),
                                GetCurrentDataVersion());
  }
  return sizes;
}

CountryFile const & Storage::GetCountryFile(CountryId const & countryId) const
{
  return CountryLeafByCountryId(countryId).GetFile();
}

LocalFilePtr Storage::GetLatestLocalFile(CountryFile const & countryFile) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryId const countryId = FindCountryIdByFile(countryFile.GetName());
  if (IsLeaf(countryId))
  {
    LocalFilePtr localFile = GetLatestLocalFile(countryId);
    if (localFile)
      return localFile;
  }

  auto const it = m_localFilesForFakeCountries.find(countryFile);
  if (it != m_localFilesForFakeCountries.end())
    return it->second;

  return LocalFilePtr();
}

LocalFilePtr Storage::GetLatestLocalFile(CountryId const & countryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const it = m_localFiles.find(countryId);
  if (it == m_localFiles.end() || it->second.empty())
    return LocalFilePtr();

  list<LocalFilePtr> const & files = it->second;
  LocalFilePtr latest = files.front();
  for (LocalFilePtr const & file : files)
  {
    if (file->GetVersion() > latest->GetVersion())
      latest = file;
  }
  return latest;
}

Status Storage::CountryStatus(CountryId const & countryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  // Check if this country has failed while downloading.
  if (m_failedCountries.count(countryId) > 0)
    return Status::EDownloadFailed;

  // Check if we are already downloading this country or have it in the queue.
  if (IsCountryInQueue(countryId))
  {
    if (IsCountryFirstInQueue(countryId))
    {
      if (IsDiffApplyingInProgressToCountry(countryId))
        return Status::EApplying;
      return Status::EDownloading;
    }

    return Status::EInQueue;
  }

  return Status::EUnknown;
}

Status Storage::CountryStatusEx(CountryId const & countryId) const
{
  return CountryStatusFull(countryId, CountryStatus(countryId));
}

void Storage::CountryStatusEx(CountryId const & countryId, Status & status,
                              MapOptions & options) const
{
  status = CountryStatusEx(countryId);

  if (status == Status::EOnDisk || status == Status::EOnDiskOutOfDate)
  {
    options = MapOptions::Map;

    LocalFilePtr localFile = GetLatestLocalFile(countryId);
    ASSERT(localFile, ("Invariant violation: local file out of sync with disk."));
    if (localFile->OnDisk(MapOptions::CarRouting))
      options = SetOptions(options, MapOptions::CarRouting);
  }
}

void Storage::SaveDownloadQueue()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (!m_keepDownloadingQueue)
    return;

  ostringstream download;
  ostringstream update;
  for (auto const & item : m_queue)
  {
    auto & ss = item.GetInitOptions() == MapOptions::Diff ? update : download;
    ss << (ss.str().empty() ? "" : ";") << item.GetCountryId();
  }

  settings::Set(kDownloadQueueKey, download.str());
  settings::Set(kUpdateQueueKey, update.str());
}

void Storage::RestoreDownloadQueue()
{
  if (!m_keepDownloadingQueue)
    return;

  string download, update;
  if (!settings::Get(kDownloadQueueKey, download) && !settings::Get(kUpdateQueueKey, update))
    return;

  auto parse = [this](string const & token, bool isUpdate) {
    if (token.empty())
      return;

    for (strings::SimpleTokenizer iter(token, ";"); iter; ++iter)
    {
      auto const diffIt = find_if(m_notAppliedDiffs.cbegin(), m_notAppliedDiffs.cend(),
                                  [this, &iter](LocalCountryFile const & localDiff) {
                                    return *iter == FindCountryIdByFile(localDiff.GetCountryName());
                                  });

      if (diffIt == m_notAppliedDiffs.end())
        DownloadNode(*iter, isUpdate);
    }
  };

  parse(download, false /* isUpdate */);
  parse(update, true /* isUpdate */);
}

void Storage::DownloadCountry(CountryId const & countryId, MapOptions opt)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

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
  {
    if (m_startDownloadingCallback)
      m_startDownloadingCallback();
    DownloadNextCountryFromQueue();
  }
  else
  {
    NotifyStatusChangedForHierarchy(countryId);
  }
  SaveDownloadQueue();
}

void Storage::DeleteCountry(CountryId const & countryId, MapOptions opt)
{
  ASSERT(m_willDelete != nullptr, ("Storage::Init wasn't called"));

  LocalFilePtr localFile = GetLatestLocalFile(countryId);
  opt = NormalizeDeleteFileSet(opt);
  bool const deferredDelete = m_willDelete(countryId, localFile);
  DeleteCountryFiles(countryId, opt, deferredDelete);
  DeleteCountryFilesFromDownloader(countryId);
  NotifyStatusChangedForHierarchy(countryId);
}

void Storage::DeleteCustomCountryVersion(LocalCountryFile const & localFile)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

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

  CountryId const countryId = FindCountryIdByFile(countryFile.GetName());
  if (!IsLeaf(countryId))
  {
    LOG(LERROR, ("Removed files for an unknown country:", localFile));
    return;
  }
}

void Storage::NotifyStatusChanged(CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  for (CountryObservers const & observer : m_observers)
    observer.m_changeCountryFn(countryId);
}

void Storage::NotifyStatusChangedForHierarchy(CountryId const & countryId)
{
  // Notification status changing for a leaf in country tree.
  NotifyStatusChanged(countryId);

  // Notification status changing for ancestors in country tree.
  ForEachAncestorExceptForTheRoot(
      countryId,
      [&](CountryId const & parentId, CountryTreeNode const &) { NotifyStatusChanged(parentId); });
}

void Storage::DownloadNextCountryFromQueue()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  bool const stopDownload = !m_downloadingPolicy->IsDownloadingAllowed();

  if (m_queue.empty())
  {
    m_diffManager.RemoveAppliedDiffs();
    m_downloadingPolicy->ScheduleRetry(m_failedCountries, [this](CountriesSet const & needReload) {
      for (auto const & country : needReload)
      {
        NodeStatuses status;
        GetNodeStatuses(country, status);
        if (status.m_error == NodeErrorCode::NoInetConnection)
          RetryDownloadNode(country);
      }
    });
    CountriesVec localMaps;
    GetLocalRealMaps(localMaps);
    GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kMapListing, localMaps);
    if (!localMaps.empty())
      GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kMapDownloadDiscovered);
    return;
  }

  QueuedCountry & queuedCountry = m_queue.front();
  CountryId const countryId = queuedCountry.GetCountryId();

  // It's not even possible to prepare directory for files before
  // downloading.  Mark this country as failed and switch to next
  // country.
  if (stopDownload ||
      !PreparePlaceForCountryFiles(GetCurrentDataVersion(), m_dataDir, GetCountryFile(countryId)))
  {
    OnMapDownloadFinished(countryId, HttpRequest::Status::Failed, queuedCountry.GetInitOptions());
    return;
  }

  DownloadNextFile(queuedCountry);

  // New status for the country, "Downloading"
  NotifyStatusChangedForHierarchy(countryId);
}

void Storage::DownloadNextFile(QueuedCountry const & country)
{
  CountryId const & countryId = country.GetCountryId();
  auto const opt = country.GetCurrentFileOptions();

  string const readyFilePath = GetFileDownloadPath(countryId, opt);
  uint64_t size;
  auto & p = GetPlatform();

  if (opt == MapOptions::Nothing)
  {
    // The diff was already downloaded (so the opt is Nothing) and
    // is being applied.
    // Still, an update of the status of another country might
    // have kicked the downloader so we're here.
    ASSERT(IsDiffApplyingInProgressToCountry(countryId), ());
    return;
  }

  // Since a downloaded valid diff file may be either with .diff or .diff.ready extension,
  // we have to check these both cases in order to find
  // the diff file which is ready to apply.
  // If there is a such file we have to cause the success download scenario.
  bool isDownloadedDiff = false;
  if (opt == MapOptions::Diff)
  {
    string filePath = readyFilePath;
    base::GetNameWithoutExt(filePath);
    isDownloadedDiff = p.GetFileSizeByFullPath(filePath, size);
  }

  // It may happen that the file already was downloaded, so there're
  // no need to request servers list and download file.  Let's
  // switch to next file.
  if (isDownloadedDiff || p.GetFileSizeByFullPath(readyFilePath, size))
  {
    OnMapFileDownloadFinished(HttpRequest::Status::Completed,
                              MapFilesDownloader::Progress(size, size));
    return;
  }

  if (m_sessionServerList || !m_downloadingUrlsForTesting.empty())
  {
    DoDownload();
  }
  else
  {
    LoadServerListForSession();
    SetDeferDownloading();
  }
}

void Storage::DeleteFromDownloader(CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (DeleteCountryFilesFromDownloader(countryId))
    NotifyStatusChangedForHierarchy(countryId);
}

bool Storage::IsDownloadInProgress() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  return !m_queue.empty();
}

CountryId Storage::GetCurrentDownloadingCountryId() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  return IsDownloadInProgress() ? m_queue.front().GetCountryId() : storage::CountryId();
}

void Storage::LoadCountriesFile(string const & pathToCountriesFile, string const & dataDir,
                                TMappingOldMwm * mapping /* = nullptr */)
{
  m_dataDir = dataDir;

  if (m_countries.IsEmpty())
  {
    m_currentVersion =
        LoadCountriesFromFile(pathToCountriesFile, m_countries, m_affiliations, mapping);
    LOG_SHORT(LINFO, ("Loaded countries list for version:", m_currentVersion));
    if (m_currentVersion < 0)
      LOG(LERROR, ("Can't load countries file", pathToCountriesFile));
  }
}

int Storage::Subscribe(ChangeCountryFunction const & change, ProgressFunction const & progress)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryObservers obs;

  obs.m_changeCountryFn = change;
  obs.m_progressFn = progress;
  obs.m_slotId = ++m_currentSlotId;

  m_observers.push_back(obs);

  return obs.m_slotId;
}

void Storage::Unsubscribe(int slotId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  for (auto i = m_observers.begin(); i != m_observers.end(); ++i)
  {
    if (i->m_slotId == slotId)
    {
      m_observers.erase(i);
      return;
    }
  }
}

void Storage::OnMapFileDownloadFinished(HttpRequest::Status status,
                                        MapFilesDownloader::Progress const & progress)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (m_queue.empty())
    return;

  bool const success = status == HttpRequest::Status::Completed;
  QueuedCountry & queuedCountry = m_queue.front();
  CountryId const countryId = queuedCountry.GetCountryId();

  if (success && queuedCountry.SwitchToNextFile())
  {
    DownloadNextFile(queuedCountry);
    return;
  }

  // Send statistics to PushWoosh. We send these statistics only for the newly downloaded
  // mwms, not for updated ones.
  if (success && queuedCountry.GetInitOptions() != MapOptions::Diff)
  {
    auto const it = m_localFiles.find(countryId);
    if (it == m_localFiles.end())
    {
      GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kMapLastDownloaded,
                                                           countryId);
      char nowStr[18]{};
      tm now = base::GmTime(time(nullptr));
      strftime(nowStr, sizeof(nowStr), "%Y-%m-%d %H:%M", &now);
      GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kMapLastDownloadedTimestamp,
                                                           std::string(nowStr));
    }
  }

  OnMapDownloadFinished(countryId, status, queuedCountry.GetInitOptions());
}

void Storage::ReportProgress(CountryId const & countryId, MapFilesDownloader::Progress const & p)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  for (CountryObservers const & o : m_observers)
    o.m_progressFn(countryId, p);
}

void Storage::ReportProgressForHierarchy(CountryId const & countryId,
                                         MapFilesDownloader::Progress const & leafProgress)
{
  // Reporting progress for a leaf in country tree.
  ReportProgress(countryId, leafProgress);

  // Reporting progress for the parents of the leaf with |countryId|.
  CountriesSet setQueue;
  GetQueuedCountries(m_queue, setQueue);

  auto calcProgress = [&](CountryId const & parentId, CountryTreeNode const & parentNode) {
    CountriesVec descendants;
    parentNode.ForEachDescendant([&descendants](CountryTreeNode const & container) {
      descendants.push_back(container.Value().Name());
    });

    MapFilesDownloader::Progress localAndRemoteBytes =
        CalculateProgress(countryId, descendants, leafProgress, setQueue);
    ReportProgress(parentId, localAndRemoteBytes);
  };

  ForEachAncestorExceptForTheRoot(countryId, calcProgress);
}

void Storage::DoDownload()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  CHECK(m_sessionServerList || !m_downloadingUrlsForTesting.empty(), ());

  // Queue can be empty because countries were deleted from queue.
  if (m_queue.empty())
    return;

  QueuedCountry & queuedCountry = m_queue.front();
  if (queuedCountry.GetInitOptions() == MapOptions::Diff)
  {
    using diffs::Status;
    auto const status = m_diffManager.GetStatus();
    switch (status)
    {
    case Status::Undefined: SetDeferDownloading(); return;
    case Status::NotAvailable:
      queuedCountry.ResetToDefaultOptions();
      break;
    case Status::Available:
      if (!m_diffManager.HasDiffFor(queuedCountry.GetCountryId()))
        queuedCountry.ResetToDefaultOptions();
      break;
    }
  }

  vector<string> const & downloadingUrls =
      m_downloadingUrlsForTesting.empty() ? *m_sessionServerList : m_downloadingUrlsForTesting;
  vector<string> fileUrls;
  fileUrls.reserve(downloadingUrls.size());
  for (string const & url : downloadingUrls)
    fileUrls.push_back(GetFileDownloadUrl(url, queuedCountry));

  string const filePath =
      GetFileDownloadPath(queuedCountry.GetCountryId(), queuedCountry.GetCurrentFileOptions());
  using namespace std::placeholders;
  m_downloader->DownloadMapFile(fileUrls, filePath, GetDownloadSize(queuedCountry),
                                bind(&Storage::OnMapFileDownloadFinished, this, _1, _2),
                                bind(&Storage::OnMapFileDownloadProgress, this, _1));
}

void Storage::SetDeferDownloading()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_needToStartDeferredDownloading = true;
}

void Storage::DoDeferredDownloadIfNeeded()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (!m_needToStartDeferredDownloading || !m_sessionServerList)
    return;

  m_needToStartDeferredDownloading = false;
  DoDownload();
}

void Storage::OnMapFileDownloadProgress(MapFilesDownloader::Progress const & progress)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  // Queue can be empty because countries were deleted from queue.
  if (m_queue.empty())
    return;

  if (m_observers.empty())
    return;

  ReportProgressForHierarchy(m_queue.front().GetCountryId(), progress);
}

void Storage::RegisterDownloadedFiles(CountryId const & countryId, MapOptions options)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const fn = [this, countryId](bool isSuccess) {
    CHECK_THREAD_CHECKER(m_threadChecker, ());
    if (!isSuccess)
    {
      OnMapDownloadFailed(countryId);
      return;
    }

    LocalFilePtr localFile = GetLocalFile(countryId, GetCurrentDataVersion());
    ASSERT(localFile, ());
    DeleteCountryIndexes(*localFile);
    m_didDownload(countryId, localFile);

    CHECK(!m_queue.empty(), ());
    PushToJustDownloaded(m_queue.begin());
    PopFromQueue(m_queue.begin());
    SaveDownloadQueue();

    m_downloader->Reset();
    NotifyStatusChangedForHierarchy(countryId);
    DownloadNextCountryFromQueue();
  };

  if (options == MapOptions::Diff)
  {
    ApplyDiff(countryId, fn);
    return;
  }

  CountryFile const countryFile = GetCountryFile(countryId);
  LocalFilePtr localFile = GetLocalFile(countryId, GetCurrentDataVersion());
  if (!localFile)
    localFile = PreparePlaceForCountryFiles(GetCurrentDataVersion(), m_dataDir, countryFile);
  if (!localFile)
  {
    LOG(LERROR, ("Local file data structure can't be prepared for downloaded file(", countryFile,
                 options, ")."));
    fn(false /* isSuccess */);
    return;
  }

  bool ok = true;
  vector<MapOptions> mapOpt = {MapOptions::Map};
  if (!version::IsSingleMwm(GetCurrentDataVersion()))
    mapOpt.emplace_back(MapOptions::CarRouting);

  for (MapOptions file : mapOpt)
  {
    if (!HasOptions(options, file))
      continue;
    string const path = GetFileDownloadPath(countryId, file);
    if (!base::RenameFileX(path, localFile->GetPath(file)))
    {
      ok = false;
      break;
    }
  }

  if (!ok)
  {
    localFile->DeleteFromDisk(options);
    fn(false);
    return;
  }

  static string const kSourceKey = "map";
  if (m_integrityValidationEnabled && !ValidateIntegrity(localFile, countryId, kSourceKey))
  {
    base::DeleteFileX(localFile->GetPath(MapOptions::Map));
    fn(false /* isSuccess */);
    return;
  }

  RegisterCountryFiles(localFile);
  fn(true);
}

void Storage::OnMapDownloadFinished(CountryId const & countryId, HttpRequest::Status status,
                                    MapOptions options)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(m_didDownload != nullptr, ("Storage::Init wasn't called"));
  ASSERT_NOT_EQUAL(MapOptions::Nothing, options,
                   ("This method should not be called for empty files set."));

  alohalytics::LogEvent("$OnMapDownloadFinished",
                        alohalytics::TStringMap(
                            {{"name", countryId},
                             {"status", status == HttpRequest::Status::Completed ? "ok" : "failed"},
                             {"version", strings::to_string(GetCurrentDataVersion())},
                             {"option", DebugPrint(options)}}));
  GetPlatform().GetMarketingService().SendMarketingEvent(marketing::kDownloaderMapActionFinished,
                                                         {{"action", "download"}});

  if (status != HttpRequest::Status::Completed)
  {
    if (status == HttpRequest::Status::FileNotFound && options == MapOptions::Diff)
    {
      m_diffManager.AbortDiffScheme();
      NotifyStatusChanged(GetRootId());
    }

    OnMapDownloadFailed(countryId);
    return;
  }

  RegisterDownloadedFiles(countryId, options);
}

string Storage::GetFileDownloadUrl(string const & baseUrl,
                                   QueuedCountry const & queuedCountry) const
{
  auto const & countryId = queuedCountry.GetCountryId();
  auto const options = queuedCountry.GetCurrentFileOptions();
  CountryFile const & countryFile = GetCountryFile(countryId);

  string const fileName = GetFileName(countryFile.GetName(), options, GetCurrentDataVersion());

  ostringstream url;
  url << baseUrl;
  string const currentVersion = strings::to_string(GetCurrentDataVersion());
  if (options == MapOptions::Diff)
  {
    uint64_t version;
    CHECK(m_diffManager.VersionFor(countryId, version), ());
    url << "diffs/" << currentVersion << "/" << strings::to_string(version);
  }
  else
  {
    url << OMIM_OS_NAME "/" << currentVersion;
  }

  url << "/" << UrlEncode(fileName);
  return url.str();
}

string Storage::GetFileDownloadUrl(string const & baseUrl, string const & fileName) const
{
  return baseUrl + OMIM_OS_NAME "/" + strings::to_string(GetCurrentDataVersion()) + "/" +
         UrlEncode(fileName);
}

CountryId Storage::FindCountryIdByFile(string const & name) const
{
  // @TODO(bykoianko) Probably it's worth to check here if name represent a node in the tree.
  return CountryId(name);
}

CountriesVec Storage::FindAllIndexesByFile(CountryId const & name) const
{
  // @TODO(bykoianko) This method should be rewritten. At list now name and the param of Find
  // have different types: string and CountryId.
  CountriesVec result;
  if (m_countries.FindFirst(name))
    result.push_back(name);
  return result;
}

void Storage::GetOutdatedCountries(vector<Country const *> & countries) const
{
  for (auto const & p : m_localFiles)
  {
    CountryId const & countryId = p.first;
    string const name = GetCountryFile(countryId).GetName();
    LocalFilePtr file = GetLatestLocalFile(countryId);
    if (file && file->GetVersion() != GetCurrentDataVersion() && name != WORLD_COASTS_FILE_NAME &&
        name != WORLD_COASTS_OBSOLETE_FILE_NAME && name != WORLD_FILE_NAME)
    {
      countries.push_back(&CountryLeafByCountryId(countryId));
    }
  }
}

Status Storage::CountryStatusWithoutFailed(CountryId const & countryId) const
{
  // First, check if we already downloading this country or have in in the queue.
  if (!IsCountryInQueue(countryId))
    return CountryStatusFull(countryId, Status::EUnknown);
  return IsCountryFirstInQueue(countryId) ? Status::EDownloading : Status::EInQueue;
}

Status Storage::CountryStatusFull(CountryId const & countryId, Status const status) const
{
  if (status != Status::EUnknown)
    return status;

  LocalFilePtr localFile = GetLatestLocalFile(countryId);
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

QueuedCountry * Storage::FindCountryInQueue(CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto it = find(m_queue.begin(), m_queue.end(), countryId);
  return it == m_queue.end() ? nullptr : &*it;
}

QueuedCountry const * Storage::FindCountryInQueue(CountryId const & countryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto it = find(m_queue.begin(), m_queue.end(), countryId);
  return it == m_queue.end() ? nullptr : &*it;
}

bool Storage::IsCountryInQueue(CountryId const & countryId) const
{
  return FindCountryInQueue(countryId) != nullptr;
}

bool Storage::IsCountryFirstInQueue(CountryId const & countryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  return !m_queue.empty() && m_queue.front().GetCountryId() == countryId;
}

bool Storage::IsDiffApplyingInProgressToCountry(CountryId const & countryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (!IsCountryFirstInQueue(countryId))
    return false;

  return m_queue.front().GetCountryId() == m_latestDiffRequest;
}

void Storage::SetLocale(string const & locale) { m_countryNameGetter.SetLocale(locale); }
string Storage::GetLocale() const { return m_countryNameGetter.GetLocale(); }
void Storage::SetDownloaderForTesting(unique_ptr<MapFilesDownloader> && downloader)
{
  m_downloader = move(downloader);
  LoadServerListForTesting();
}

void Storage::SetEnabledIntegrityValidationForTesting(bool enabled)
{
  m_integrityValidationEnabled = enabled;
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

LocalFilePtr Storage::GetLocalFile(CountryId const & countryId, int64_t version) const
{
  auto const it = m_localFiles.find(countryId);
  if (it == m_localFiles.end() || it->second.empty())
    return LocalFilePtr();

  for (auto const & file : it->second)
  {
    if (file->GetVersion() == version)
      return file;
  }
  return LocalFilePtr();
}

void Storage::RegisterCountryFiles(LocalFilePtr localFile)
{
  CHECK(localFile, ());
  localFile->SyncWithDisk();

  for (auto const & countryId : FindAllIndexesByFile(localFile->GetCountryName()))
  {
    LocalFilePtr existingFile = GetLocalFile(countryId, localFile->GetVersion());
    if (existingFile)
      ASSERT_EQUAL(localFile.get(), existingFile.get(), ());
    else
      m_localFiles[countryId].push_front(localFile);
  }
}

void Storage::RegisterCountryFiles(CountryId const & countryId, string const & directory,
                                   int64_t version)
{
  LocalFilePtr localFile = GetLocalFile(countryId, version);
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

  LocalFilePtr fakeCountryLocalFile = make_shared<LocalCountryFile>(localFile);
  fakeCountryLocalFile->SyncWithDisk();
  m_localFilesForFakeCountries[fakeCountryLocalFile->GetCountryFile()] = fakeCountryLocalFile;
}

void Storage::DeleteCountryFiles(CountryId const & countryId, MapOptions opt, bool deferredDelete)
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
  auto isNull = [](LocalFilePtr const & localFile) { return !localFile; };
  localFiles.remove_if(isNull);
  if (localFiles.empty())
    m_localFiles.erase(countryId);
}

bool Storage::DeleteCountryFilesFromDownloader(CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  QueuedCountry * queuedCountry = FindCountryInQueue(countryId);
  if (!queuedCountry)
    return false;

  if (m_latestDiffRequest && m_latestDiffRequest == countryId)
    m_diffsCancellable.Cancel();

  MapOptions const opt = queuedCountry->GetInitOptions();
  if (IsCountryFirstInQueue(countryId))
  {
    // Abrupt downloading of the current file if it should be removed.
    if (HasOptions(opt, queuedCountry->GetCurrentFileOptions()))
      m_downloader->Reset();

    // Remove all files the downloader has created for the country.
    DeleteDownloaderFilesForCountry(GetCurrentDataVersion(), m_dataDir, GetCountryFile(countryId));
  }

  queuedCountry->RemoveOptions(opt);

  // Remove country from the queue if there's nothing to download.
  if (queuedCountry->GetInitOptions() == MapOptions::Nothing)
  {
    auto it = find(m_queue.begin(), m_queue.end(), countryId);
    ASSERT(it != m_queue.end(), ());
    PopFromQueue(it);
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
  CountryId const & countryId = queuedCountry.GetCountryId();
  uint64_t size;
  if (queuedCountry.GetInitOptions() == MapOptions::Diff)
  {
    CHECK_NOT_EQUAL(m_diffManager.SizeFor(countryId, size), 0, ());
    return size;
  }

  CountryFile const & file = GetCountryFile(countryId);
  return GetRemoteSize(file, queuedCountry.GetCurrentFileOptions(), GetCurrentDataVersion());
}

string Storage::GetFileDownloadPath(CountryId const & countryId, MapOptions options) const
{
  return platform::GetFileDownloadPath(GetCurrentDataVersion(), m_dataDir,
                                       GetCountryFile(countryId), options);
}

bool Storage::CheckFailedCountries(CountriesVec const & countries) const
{
  for (auto const & country : countries)
  {
    if (m_failedCountries.count(country))
      return true;
  }
  return false;
}

CountryId const Storage::GetRootId() const { return m_countries.GetRoot().Value().Name(); }
void Storage::GetChildren(CountryId const & parent, CountriesVec & childIds) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryTreeNode const * const parentNode = m_countries.FindFirst(parent);
  if (parentNode == nullptr)
  {
    ASSERT(false, ("CountryId =", parent, "not found in m_countries."));
    return;
  }

  size_t const childrenCount = parentNode->ChildrenCount();
  childIds.clear();
  childIds.reserve(childrenCount);
  for (size_t i = 0; i < childrenCount; ++i)
    childIds.emplace_back(parentNode->Child(i).Value().Name());
}

void Storage::GetLocalRealMaps(CountriesVec & localMaps) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  localMaps.clear();
  localMaps.reserve(m_localFiles.size());

  for (auto const & keyValue : m_localFiles)
    localMaps.push_back(keyValue.first);
}

void Storage::GetChildrenInGroups(CountryId const & parent, CountriesVec & downloadedChildren,
                                  CountriesVec & availChildren, bool keepAvailableChildren) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryTreeNode const * const parentNode = m_countries.FindFirst(parent);
  if (parentNode == nullptr)
  {
    ASSERT(false, ("CountryId =", parent, "not found in m_countries."));
    return;
  }

  downloadedChildren.clear();
  availChildren.clear();

  // Vector of disputed territories which are downloaded but the other maps
  // in their group are not downloaded. Only mwm in subtree with root == |parent|
  // are taken into account.
  CountriesVec disputedTerritoriesWithoutSiblings;
  // All disputed territories in subtree with root == |parent|.
  CountriesVec allDisputedTerritories;
  parentNode->ForEachChild([&](CountryTreeNode const & childNode) {
    vector<pair<CountryId, NodeStatus>> disputedTerritoriesAndStatus;
    StatusAndError const childStatus = GetNodeStatusInfo(childNode, disputedTerritoriesAndStatus,
                                                         true /* isDisputedTerritoriesCounted */);

    CountryId const & childValue = childNode.Value().Name();
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

  CountriesVec uniqueDisputed(disputedTerritoriesWithoutSiblings.begin(),
                              disputedTerritoriesWithoutSiblings.end());
  base::SortUnique(uniqueDisputed);

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

bool Storage::IsNodeDownloaded(CountryId const & countryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  for (auto const & localeMap : m_localFiles)
  {
    if (countryId == localeMap.first)
      return true;
  }
  return false;
}

bool Storage::HasLatestVersion(CountryId const & countryId) const
{
  return CountryStatusEx(countryId) == Status::EOnDisk;
}

void Storage::DownloadNode(CountryId const & countryId, bool isUpdate /* = false */)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryTreeNode const * const node = m_countries.FindFirst(countryId);

  if (!node)
    return;

  if (GetNodeStatus(*node).status == NodeStatus::OnDisk)
    return;

  auto downloadAction = [this, isUpdate](CountryTreeNode const & descendantNode) {
    if (descendantNode.ChildrenCount() == 0 &&
        GetNodeStatus(descendantNode).status != NodeStatus::OnDisk)
      this->DownloadCountry(descendantNode.Value().Name(),
                            isUpdate ? MapOptions::Diff : MapOptions::MapWithCarRouting);
  };

  node->ForEachInSubtree(downloadAction);
}

void Storage::DeleteNode(CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryTreeNode const * const node = m_countries.FindFirst(countryId);

  if (!node)
    return;

  auto deleteAction = [this](CountryTreeNode const & descendantNode) {
    bool onDisk = m_localFiles.find(descendantNode.Value().Name()) != m_localFiles.end();
    if (descendantNode.ChildrenCount() == 0 && onDisk)
      this->DeleteCountry(descendantNode.Value().Name(), MapOptions::MapWithCarRouting);
  };
  node->ForEachInSubtree(deleteAction);
}

StatusAndError Storage::GetNodeStatus(CountryTreeNode const & node) const
{
  vector<pair<CountryId, NodeStatus>> disputedTerritories;
  return GetNodeStatusInfo(node, disputedTerritories, false /* isDisputedTerritoriesCounted */);
}

bool Storage::IsDisputed(CountryTreeNode const & node) const
{
  vector<CountryTreeNode const *> found;
  m_countries.Find(node.Value().Name(), found);
  return found.size() > 1;
}

void Storage::CalcMaxMwmSizeBytes()
{
  m_maxMwmSizeBytes = 0;
  m_countries.GetRoot().ForEachInSubtree([&](CountryTree::Node const & node) {
    if (node.ChildrenCount() == 0)
    {
      MwmSize mwmSizeBytes = node.Value().GetSubtreeMwmSizeBytes();
      if (mwmSizeBytes > m_maxMwmSizeBytes)
        m_maxMwmSizeBytes = mwmSizeBytes;
    }
  });
}

void Storage::LoadDiffScheme()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  diffs::LocalMapsInfo localMapsInfo;
  auto const currentVersion = GetCurrentDataVersion();
  localMapsInfo.m_currentDataVersion = currentVersion;

  CountriesVec localMaps;
  GetLocalRealMaps(localMaps);
  for (auto const & countryId : localMaps)
  {
    auto const localFile = GetLatestLocalFile(countryId);
    auto const mapVersion = localFile->GetVersion();
    if (mapVersion != currentVersion && mapVersion > 0)
      localMapsInfo.m_localMaps.emplace(localFile->GetCountryName(), mapVersion);
  }

  m_diffManager.AddObserver(*this);
  m_diffManager.Load(move(localMapsInfo));
}

void Storage::ApplyDiff(CountryId const & countryId, function<void(bool isSuccess)> const & fn)
{
  m_diffsCancellable.Reset();
  m_latestDiffRequest = countryId;
  NotifyStatusChangedForHierarchy(countryId);

  diffs::Manager::ApplyDiffParams params;
  params.m_diffFile =
      PreparePlaceForCountryFiles(GetCurrentDataVersion(), m_dataDir, GetCountryFile(countryId));
  params.m_diffReadyPath = GetFileDownloadPath(countryId, MapOptions::Diff);

  uint64_t version;
  if (!m_diffManager.VersionFor(countryId, version))
  {
    ASSERT(false, ("Invalid attempt to get version of diff with country id:", countryId));
    fn(false);
    m_latestDiffRequest = {};
    return;
  }

  params.m_oldMwmFile = GetLocalFile(countryId, version);

  LocalFilePtr & diffFile = params.m_diffFile;

  m_diffManager.ApplyDiff(
      move(params), m_diffsCancellable,
      [this, fn, countryId, diffFile](DiffApplicationResult result) {
        static string const kSourceKey = "diff";
        if (result == DiffApplicationResult::Ok && m_integrityValidationEnabled &&
            !ValidateIntegrity(diffFile, diffFile->GetCountryName(), kSourceKey))
        {
          base::DeleteFileX(diffFile->GetPath(MapOptions::Map));
          result = DiffApplicationResult::Failed;
        }

        GetPlatform().RunTask(Platform::Thread::Gui, [this, fn, diffFile, countryId, result] {
          m_latestDiffRequest = {};
          switch (result)
          {
          case DiffApplicationResult::Ok:
          {
            RegisterCountryFiles(diffFile);
            Platform::DisableBackupForFile(diffFile->GetPath(MapOptions::Map));
            fn(true);
          }
          break;
          case DiffApplicationResult::Cancelled:
          {
            CHECK(!IsDiffApplyingInProgressToCountry(countryId), ());
          }
          break;
          case DiffApplicationResult::Failed: fn(false); break;
          }
        });
      });
}

void Storage::LoadServerListForSession()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_downloader->GetServersList([this](vector<string> const & urls) { PingServerList(urls); });
}

void Storage::LoadServerListForTesting()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_downloader->GetServersList([this](auto const & urls) { m_sessionServerList = urls; });
}

void Storage::PingServerList(vector<string> const & urls)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (urls.empty())
    return;

  GetPlatform().RunTask(Platform::Thread::Network, [urls, this] {
    Pinger::Ping(urls, [this, urls](vector<string> readyUrls) {
      CHECK_THREAD_CHECKER(m_threadChecker, ());

      if (readyUrls.empty())
        m_sessionServerList = urls;
      else
        m_sessionServerList = move(readyUrls);

      DoDeferredDownloadIfNeeded();
    });
  });
}

bool Storage::IsPossibleToAutoupdate() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  return m_diffManager.IsPossibleToAutoupdate();
}

void Storage::SetStartDownloadingCallback(StartDownloadingCallback const & cb)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_startDownloadingCallback = cb;
}

void Storage::OnDiffStatusReceived(diffs::Status const status)
{
  if (status != diffs::Status::NotAvailable)
  {
    for (auto const & localDiff : m_notAppliedDiffs)
    {
      auto const countryId = FindCountryIdByFile(localDiff.GetCountryName());

      if (m_diffManager.HasDiffFor(countryId))
        UpdateNode(countryId);
      else
        localDiff.DeleteFromDisk(MapOptions::Diff);
    }

    m_notAppliedDiffs.clear();
  }

  DoDeferredDownloadIfNeeded();
}

StatusAndError Storage::GetNodeStatusInfo(
    CountryTreeNode const & node, vector<pair<CountryId, NodeStatus>> & disputedTerritories,
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

  auto groupStatusCalculator = [&](CountryTreeNode const & nodeInSubtree) {
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

void Storage::GetNodeAttrs(CountryId const & countryId, NodeAttrs & nodeAttrs) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  vector<CountryTreeNode const *> nodes;
  m_countries.Find(countryId, nodes);
  CHECK(!nodes.empty(), (countryId));
  // If nodes.size() > 1 countryId corresponds to a disputed territories.
  // In that case it's guaranteed that most of attributes are equal for
  // each element of nodes. See Country class description for further details.
  CountryTreeNode const * const node = nodes[0];

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
  if (nodeAttrs.m_status == NodeStatus::OnDisk || nodeAttrs.m_status == NodeStatus::Applying)
  {
    // Group or leaf node is on disk and up to date.
    MwmSize const subTreeSizeBytes = node->Value().GetSubtreeMwmSizeBytes();
    nodeAttrs.m_downloadingProgress.first = subTreeSizeBytes;
    nodeAttrs.m_downloadingProgress.second = subTreeSizeBytes;
  }
  else
  {
    CountriesVec subtree;
    node->ForEachInSubtree(
        [&subtree](CountryTreeNode const & d) { subtree.push_back(d.Value().Name()); });
    CountryId const & downloadingMwm =
        IsDownloadInProgress() ? GetCurrentDownloadingCountryId() : kInvalidCountryId;
    MapFilesDownloader::Progress downloadingMwmProgress(0, 0);
    if (!m_downloader->IsIdle())
    {
      downloadingMwmProgress = m_downloader->GetDownloadingProgress();
      // If we don't know estimated file size then we ignore its progress.
      if (downloadingMwmProgress.second == -1)
        downloadingMwmProgress = {0, 0};
    }

    CountriesSet setQueue;
    GetQueuedCountries(m_queue, setQueue);
    nodeAttrs.m_downloadingProgress =
        CalculateProgress(downloadingMwm, subtree, downloadingMwmProgress, setQueue);
  }

  // Local mwm information and information about downloading mwms.
  nodeAttrs.m_localMwmCounter = 0;
  nodeAttrs.m_localMwmSize = 0;
  nodeAttrs.m_downloadingMwmCounter = 0;
  nodeAttrs.m_downloadingMwmSize = 0;
  CountriesSet visitedLocalNodes;
  node->ForEachInSubtree([this, &nodeAttrs, &visitedLocalNodes](CountryTreeNode const & d) {
    CountryId const countryId = d.Value().Name();
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
    LocalFilePtr const localFile = GetLatestLocalFile(countryId);
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
      nodes, [&](CountryId const & ancestorId, CountryTreeNode const & node) {
        if (node.Value().GetParent() == GetRootId())
          nodeAttrs.m_topmostParentInfo.push_back({ancestorId, m_countryNameGetter(ancestorId)});
      });
}

void Storage::GetNodeStatuses(CountryId const & countryId, NodeStatuses & nodeStatuses) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryTreeNode const * const node = m_countries.FindFirst(countryId);
  CHECK(node, (countryId));

  StatusAndError statusAndErr = GetNodeStatus(*node);
  nodeStatuses.m_status = statusAndErr.status;
  nodeStatuses.m_error = statusAndErr.error;
  nodeStatuses.m_groupNode = (node->ChildrenCount() != 0);
}

void Storage::SetCallbackForClickOnDownloadMap(DownloadFn & downloadFn)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_downloadMapOnTheMap = downloadFn;
}

void Storage::DoClickOnDownloadMap(CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (m_downloadMapOnTheMap)
    m_downloadMapOnTheMap(countryId);
}

MapFilesDownloader::Progress Storage::CalculateProgress(
    CountryId const & downloadingMwm, CountriesVec const & mwms,
    MapFilesDownloader::Progress const & downloadingMwmProgress,
    CountriesSet const & mwmsInQueue) const
{
  // Function calculates progress correctly ONLY if |downloadingMwm| is leaf.

  MapFilesDownloader::Progress localAndRemoteBytes = make_pair(0, 0);

  for (auto const & d : mwms)
  {
    if (downloadingMwm == d && downloadingMwm != kInvalidCountryId)
    {
      localAndRemoteBytes.first += downloadingMwmProgress.first;
      localAndRemoteBytes.second +=
          GetRemoteSize(GetCountryFile(d), MapOptions::Map, GetCurrentDataVersion());
    }
    else if (mwmsInQueue.count(d) != 0)
    {
      localAndRemoteBytes.second +=
          GetRemoteSize(GetCountryFile(d), MapOptions::Map, GetCurrentDataVersion());
    }
    else if (m_justDownloaded.count(d) != 0)
    {
      MwmSize const localCountryFileSz =
          GetRemoteSize(GetCountryFile(d), MapOptions::Map, GetCurrentDataVersion());
      localAndRemoteBytes.first += localCountryFileSz;
      localAndRemoteBytes.second += localCountryFileSz;
    }
  }

  return localAndRemoteBytes;
}

void Storage::UpdateNode(CountryId const & countryId)
{
  ForEachInSubtree(countryId, [this](CountryId const & descendantId, bool groupNode) {
    if (!groupNode && m_localFiles.find(descendantId) != m_localFiles.end())
      this->DownloadNode(descendantId, true /* isUpdate */);
  });
}

void Storage::CancelDownloadNode(CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountriesSet setQueue;
  GetQueuedCountries(m_queue, setQueue);

  ForEachInSubtree(countryId, [&](CountryId const & descendantId, bool /* groupNode */) {
    if (IsDiffApplyingInProgressToCountry(descendantId))
      m_diffsCancellable.Cancel();

    if (setQueue.count(descendantId) != 0)
      DeleteFromDownloader(descendantId);

    if (m_failedCountries.count(descendantId) != 0)
    {
      m_failedCountries.erase(descendantId);
      NotifyStatusChangedForHierarchy(countryId);
    }
  });
}

void Storage::RetryDownloadNode(CountryId const & countryId)
{
  ForEachInSubtree(countryId, [this](CountryId const & descendantId, bool groupNode) {
    if (!groupNode && m_failedCountries.count(descendantId) != 0)
    {
      bool const isUpdateRequest = m_diffManager.HasDiffFor(descendantId);
      DownloadNode(descendantId, isUpdateRequest);
    }
  });
}

bool Storage::GetUpdateInfo(CountryId const & countryId, UpdateInfo & updateInfo) const
{
  auto const updateInfoAccumulator = [&updateInfo, this](CountryTreeNode const & node) {
    if (node.ChildrenCount() != 0 || GetNodeStatus(node).status != NodeStatus::OnDiskOutOfDate)
      return;

    updateInfo.m_numberOfMwmFilesToUpdate += 1;  // It's not a group mwm.
    if (m_diffManager.HasDiffFor(node.Value().Name()))
    {
      uint64_t size;
      m_diffManager.SizeToDownloadFor(node.Value().Name(), size);
      updateInfo.m_totalUpdateSizeInBytes += size;
    }
    else
    {
      updateInfo.m_totalUpdateSizeInBytes += node.Value().GetSubtreeMwmSizeBytes();
    }

    LocalAndRemoteSize sizes =
        CountrySizeInBytes(node.Value().Name(), MapOptions::MapWithCarRouting);
    updateInfo.m_sizeDifference +=
        static_cast<int64_t>(sizes.second) - static_cast<int64_t>(sizes.first);
  };

  CountryTreeNode const * const node = m_countries.FindFirst(countryId);
  if (!node)
  {
    ASSERT(false, ());
    return false;
  }
  updateInfo = UpdateInfo();
  node->ForEachInSubtree(updateInfoAccumulator);
  return true;
}

void Storage::PushToJustDownloaded(Queue::iterator justDownloadedItem)
{
  m_justDownloaded.insert(justDownloadedItem->GetCountryId());
}

void Storage::PopFromQueue(Queue::iterator it)
{
  CHECK(!m_queue.empty(), ());
  m_queue.erase(it);
  if (m_queue.empty())
    m_justDownloaded.clear();
}

void Storage::GetQueuedChildren(CountryId const & parent, CountriesVec & queuedChildren) const
{
  CountryTreeNode const * const node = m_countries.FindFirst(parent);
  if (!node)
  {
    ASSERT(false, ());
    return;
  }

  queuedChildren.clear();
  node->ForEachChild([&queuedChildren, this](CountryTreeNode const & child) {
    NodeStatus status = GetNodeStatus(child).status;
    ASSERT_NOT_EQUAL(status, NodeStatus::Undefined, ());
    if (status == NodeStatus::Downloading || status == NodeStatus::InQueue)
      queuedChildren.push_back(child.Value().Name());
  });
}

void Storage::GetGroupNodePathToRoot(CountryId const & groupNode, CountriesVec & path) const
{
  path.clear();

  vector<CountryTreeNode const *> nodes;
  m_countries.Find(groupNode, nodes);
  if (nodes.empty())
  {
    LOG(LWARNING, ("CountryId =", groupNode, "not found in m_countries."));
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
      nodes, [&path](CountryId const & id, CountryTreeNode const &) { path.push_back(id); });
  path.push_back(m_countries.GetRoot().Value().Name());
}

void Storage::GetTopmostNodesFor(CountryId const & countryId, CountriesVec & nodes,
                                 size_t level) const
{
  nodes.clear();

  vector<CountryTreeNode const *> treeNodes;
  m_countries.Find(countryId, treeNodes);
  if (treeNodes.empty())
  {
    LOG(LWARNING, ("CountryId =", countryId, "not found in m_countries."));
    return;
  }

  nodes.resize(treeNodes.size());
  for (size_t i = 0; i < treeNodes.size(); ++i)
  {
    nodes[i] = countryId;
    CountriesVec path;
    ForEachAncestorExceptForTheRoot(
        {treeNodes[i]},
        [&path](CountryId const & id, CountryTreeNode const &) { path.emplace_back(id); });
    if (!path.empty() && level < path.size())
      nodes[i] = path[path.size() - 1 - level];
  }
}

CountryId const Storage::GetParentIdFor(CountryId const & countryId) const
{
  vector<CountryTreeNode const *> nodes;
  m_countries.Find(countryId, nodes);
  if (nodes.empty())
  {
    LOG(LWARNING, ("CountryId =", countryId, "not found in m_countries."));
    return string();
  }

  if (nodes.size() > 1)
  {
    // Disputed territory. Has multiple parents.
    return string();
  }

  return nodes[0]->Value().GetParent();
}

MwmSize Storage::GetRemoteSize(CountryFile const & file, MapOptions opt, int64_t version) const
{
  if (version::IsSingleMwm(version))
  {
    if (opt == MapOptions::Nothing)
      return 0;
    uint64_t size;
    if (m_diffManager.SizeFor(file.GetName(), size))
      return size;
    return file.GetRemoteSize(MapOptions::Map);
  }

  MwmSize size = 0;
  for (MapOptions bit : {MapOptions::Map, MapOptions::CarRouting})
  {
    if (HasOptions(opt, bit))
      size += file.GetRemoteSize(bit);
  }
  return size;
}

void Storage::OnMapDownloadFailed(CountryId const & countryId)
{
  m_failedCountries.insert(countryId);
  auto it = find(m_queue.begin(), m_queue.end(), countryId);
  if (it != m_queue.end())
    PopFromQueue(it);
  NotifyStatusChangedForHierarchy(countryId);
  DownloadNextCountryFromQueue();
}
}  // namespace storage
