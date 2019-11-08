#include "storage/storage.hpp"

#include "storage/diff_scheme/diff_scheme_loader.hpp"
#include "storage/http_map_files_downloader.hpp"

#include "defines.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/marketing_service.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include "coding/internal/file_data.hpp"

#include "base/exception.hpp"
#include "base/file_name_utils.hpp"
#include "base/gmtime.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <chrono>
#include <sstream>

#include <limits>

#include "3party/Alohalytics/src/alohalytics.h"

using namespace downloader;
using namespace generator::mwm_diff;
using namespace platform;
using namespace std;
using namespace std::chrono;
using namespace std::placeholders;

namespace storage
{
namespace
{
string const kUpdateQueueKey = "UpdateQueue";
string const kDownloadQueueKey = "DownloadQueue";

void DeleteCountryIndexes(LocalCountryFile const & localFile)
{
  platform::CountryIndexes::DeleteFromDisk(localFile);
}

void DeleteFromDiskWithIndexes(LocalCountryFile const & localFile, MapFileType type)
{
  DeleteCountryIndexes(localFile);
  localFile.DeleteFromDisk(type);
}

CountryTree::Node const & LeafNodeFromCountryId(CountryTree const & root,
                                                CountryId const & countryId)
{
  CountryTree::Node const * node = root.FindFirstLeaf(countryId);
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
  , m_dataDir(dataDir)
{
  SetLocale(languages::GetCurrentTwine());
  LoadCountriesFile(pathToCountriesFile);
  CalcMaxMwmSizeBytes();
}

Storage::Storage(string const & referenceCountriesTxtJsonForTesting,
                 unique_ptr<MapFilesDownloader> mapDownloaderForTesting)
  : m_downloader(move(mapDownloaderForTesting))
{
  m_currentVersion =
      LoadCountriesFromBuffer(referenceCountriesTxtJsonForTesting, m_countries, m_affiliations,
                              m_countryNameSynonyms, m_mwmTopCityGeoIds, m_mwmTopCountryGeoIds);
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
      DeleteFromDiskWithIndexes(*localFile, MapFileType::Map);
      DeleteFromDiskWithIndexes(*localFile, MapFileType::Diff);
    }
  }
}

bool Storage::HaveDownloadedCountries() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  return !m_localFiles.empty();
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

      DeleteFromDiskWithIndexes(localFile, MapFileType::Map);
      DeleteFromDiskWithIndexes(localFile, MapFileType::Diff);
      ++j;
    }

    LocalCountryFile const & localFile = *i;
    string const & name = localFile.GetCountryName();
    if (name != WORLD_FILE_NAME && name != WORLD_COASTS_FILE_NAME)
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
  // Note: call order is important, diffs loading must be called first.
  // Since diffs downloading and servers list downloading
  // are working on network thread, consecutive executing is guaranteed.
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
  CountryTree::Node const * node = m_countries.FindFirst(countryId);
  CHECK(node, ("Node with id =", countryId, "not found in country tree."));
  return node->Value();
}

bool Storage::IsNode(CountryId const & countryId) const
{
  if (!IsCountryIdValid(countryId))
    return false;
  return m_countries.FindFirst(countryId) != nullptr;
}

bool Storage::IsLeaf(CountryId const & countryId) const
{
  if (!IsCountryIdValid(countryId))
    return false;
  CountryTree::Node const * const node = m_countries.FindFirst(countryId);
  return node != nullptr && node->ChildrenCount() == 0;
}

bool Storage::IsInnerNode(CountryId const & countryId) const
{
  if (!IsCountryIdValid(countryId))
    return false;
  CountryTree::Node const * const node = m_countries.FindFirst(countryId);
  return node != nullptr && node->ChildrenCount() != 0;
}

LocalAndRemoteSize Storage::CountrySizeInBytes(CountryId const & countryId) const
{
  QueuedCountry const * queuedCountry = FindCountryInQueue(countryId);
  LocalFilePtr localFile = GetLatestLocalFile(countryId);
  CountryFile const & countryFile = GetCountryFile(countryId);
  if (queuedCountry == nullptr)
  {
    return LocalAndRemoteSize(localFile ? localFile->GetSize(MapFileType::Map) : 0,
                              GetRemoteSize(countryFile, GetCurrentDataVersion()));
  }

  LocalAndRemoteSize sizes(0, GetRemoteSize(countryFile, GetCurrentDataVersion()));
  if (!m_downloader->IsIdle() && IsCountryFirstInQueue(countryId))
  {
    sizes.first = m_downloader->GetDownloadingProgress().first +
                  GetRemoteSize(countryFile, GetCurrentDataVersion());
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
  auto const status = CountryStatus(countryId);
  if (status != Status::EUnknown)
    return status;

  auto localFile = GetLatestLocalFile(countryId);
  if (!localFile || !localFile->OnDisk(MapFileType::Map))
    return Status::ENotDownloaded;

  auto const & countryFile = GetCountryFile(countryId);
  if (GetRemoteSize(countryFile, GetCurrentDataVersion()) == 0)
    return Status::EUnknown;

  if (localFile->GetVersion() != GetCurrentDataVersion())
    return Status::EOnDiskOutOfDate;
  return Status::EOnDisk;
}

void Storage::CountryStatusEx(CountryId const & countryId, Status & status,
                              MapFileType & type) const
{
  status = CountryStatusEx(countryId);

  if (status == Status::EOnDisk || status == Status::EOnDiskOutOfDate)
  {
    type = MapFileType::Map;
    ASSERT(GetLatestLocalFile(countryId),
           ("Invariant violation: local file out of sync with disk."));
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
    auto & ss = item.GetFileType() == MapFileType::Diff ? update : download;
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

void Storage::DownloadCountry(CountryId const & countryId, MapFileType type)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (FindCountryInQueue(countryId) != nullptr)
    return;

  m_failedCountries.erase(countryId);
  m_queue.push_back(QueuedCountry(countryId, type));
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

void Storage::DeleteCountry(CountryId const & countryId, MapFileType type)
{
  ASSERT(m_willDelete != nullptr, ("Storage::Init wasn't called"));

  LocalFilePtr localFile = GetLatestLocalFile(countryId);
  bool const deferredDelete = m_willDelete(countryId, localFile);
  DeleteCountryFiles(countryId, type, deferredDelete);
  DeleteCountryFilesFromDownloader(countryId);
  m_diffManager.RemoveDiffForCountry(countryId);

  NotifyStatusChangedForHierarchy(countryId);
}

void Storage::DeleteCustomCountryVersion(LocalCountryFile const & localFile)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryFile const countryFile = localFile.GetCountryFile();
  DeleteFromDiskWithIndexes(localFile, MapFileType::Map);
  DeleteFromDiskWithIndexes(localFile, MapFileType::Diff);

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
  ForEachAncestorExceptForTheRoot(countryId,
                                  [&](CountryId const & parentId, CountryTree::Node const &) {
                                    NotifyStatusChanged(parentId);
                                  });
}

void Storage::DownloadNextCountryFromQueue()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  bool const stopDownload = !m_downloadingPolicy->IsDownloadingAllowed();

  if (m_queue.empty())
  {
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
    OnMapDownloadFinished(countryId, HttpRequest::Status::Failed, queuedCountry.GetFileType());
    return;
  }

  DownloadNextFile(queuedCountry);

  // New status for the country, "Downloading"
  NotifyStatusChangedForHierarchy(countryId);
}

void Storage::DownloadNextFile(QueuedCountry const & country)
{
  CountryId const & countryId = country.GetCountryId();
  auto const opt = country.GetFileType();

  string const readyFilePath = GetFileDownloadPath(countryId, opt);
  uint64_t size;
  auto & p = GetPlatform();

  // Since a downloaded valid diff file may be either with .diff or .diff.ready extension,
  // we have to check these both cases in order to find
  // the diff file which is ready to apply.
  // If there is a such file we have to cause the success download scenario.
  bool isDownloadedDiff = false;
  if (opt == MapFileType::Diff)
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
    if (m_latestDiffRequest)
    {
      // No need to kick the downloader: it will be kicked by
      // the current diff application process when it is completed or cancelled.
      return;
    }
    OnMapFileDownloadFinished(HttpRequest::Status::Completed,
                              MapFilesDownloader::Progress(size, size));
    return;
  }

  DoDownload();
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

void Storage::LoadCountriesFile(string const & pathToCountriesFile)
{
  if (m_countries.IsEmpty())
  {
    m_currentVersion =
        LoadCountriesFromFile(pathToCountriesFile, m_countries, m_affiliations,
                              m_countryNameSynonyms, m_mwmTopCityGeoIds, m_mwmTopCountryGeoIds);
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

  // Send statistics to PushWoosh. We send these statistics only for the newly downloaded
  // mwms, not for updated ones.
  if (success && queuedCountry.GetFileType() != MapFileType::Diff)
  {
    auto const it = m_localFiles.find(countryId);
    if (it == m_localFiles.end())
    {
      GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kMapLastDownloaded,
                                                           countryId);
      auto const nowStr = GetPlatform().GetMarketingService().GetPushWooshTimestamp();
      GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kMapLastDownloadedTimestamp,
                                                           nowStr);
    }
  }

  OnMapDownloadFinished(countryId, status, queuedCountry.GetFileType());
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

  auto calcProgress = [&](CountryId const & parentId, CountryTree::Node const & parentNode) {
    CountriesVec descendants;
    parentNode.ForEachDescendant([&descendants](CountryTree::Node const & container) {
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

  // Queue can be empty because countries were deleted from queue.
  if (m_queue.empty())
    return;

  QueuedCountry & queuedCountry = m_queue.front();
  if (queuedCountry.GetFileType() == MapFileType::Diff)
  {
    using diffs::Status;
    auto const status = m_diffManager.GetStatus();
    switch (status)
    {
    case Status::Undefined:
      SetDeferDownloading();
      return;
    case Status::NotAvailable:
      queuedCountry.SetFileType(MapFileType::Map);
      break;
    case Status::Available:
      if (!m_diffManager.HasDiffFor(queuedCountry.GetCountryId()))
        queuedCountry.SetFileType(MapFileType::Map);
      break;
    }
  }

  auto const & id = queuedCountry.GetCountryId();
  auto const options = queuedCountry.GetFileType();
  auto const relativeUrl = GetDownloadRelativeUrl(id, options);
  auto const filePath = GetFileDownloadPath(id, options);

  m_downloader->DownloadMapFile(relativeUrl, filePath, GetDownloadSize(queuedCountry),
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

  if (!m_needToStartDeferredDownloading)
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

void Storage::RegisterDownloadedFiles(CountryId const & countryId, MapFileType type)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const fn = [this, countryId, type](bool isSuccess) {
    CHECK_THREAD_CHECKER(m_threadChecker, ());

    LOG(LINFO, ("Registering downloaded file:", countryId, type, "; success:", isSuccess));

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

  if (type == MapFileType::Diff)
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
                 type, ")."));
    fn(false /* isSuccess */);
    return;
  }

  string const path = GetFileDownloadPath(countryId, type);
  if (!base::RenameFileX(path, localFile->GetPath(type)))
  {
    localFile->DeleteFromDisk(type);
    fn(false);
    return;
  }

  static string const kSourceKey = "map";
  if (m_integrityValidationEnabled && !ValidateIntegrity(localFile, countryId, kSourceKey))
  {
    base::DeleteFileX(localFile->GetPath(MapFileType::Map));
    fn(false /* isSuccess */);
    return;
  }

  RegisterCountryFiles(localFile);
  fn(true);
}

void Storage::OnMapDownloadFinished(CountryId const & countryId, HttpRequest::Status status,
                                    MapFileType type)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(m_didDownload != nullptr, ("Storage::Init wasn't called"));

  alohalytics::LogEvent("$OnMapDownloadFinished",
                        alohalytics::TStringMap(
                            {{"name", countryId},
                             {"status", status == HttpRequest::Status::Completed ? "ok" : "failed"},
                             {"version", strings::to_string(GetCurrentDataVersion())},
                             {"option", DebugPrint(type)}}));
  GetPlatform().GetMarketingService().SendMarketingEvent(marketing::kDownloaderMapActionFinished,
                                                         {{"action", "download"}});

  if (status != HttpRequest::Status::Completed)
  {
    if (status == HttpRequest::Status::FileNotFound && type == MapFileType::Diff)
    {
      m_diffManager.AbortDiffScheme();
      NotifyStatusChanged(GetRootId());
    }

    OnMapDownloadFailed(countryId);
    return;
  }

  RegisterDownloadedFiles(countryId, type);
}

string Storage::GetDownloadRelativeUrl(CountryId const & countryId, MapFileType type) const
{
  auto const & countryFile = GetCountryFile(countryId);
  auto const fileName = GetFileName(countryFile.GetName(), type);

  uint64_t diffVersion = 0;
  if (type == MapFileType::Diff)
    CHECK(m_diffManager.VersionFor(countryId, diffVersion), ());

  return MapFilesDownloader::MakeRelativeUrl(fileName, GetCurrentDataVersion(), diffVersion);
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
    if (file && file->GetVersion() != GetCurrentDataVersion() && name != WORLD_COASTS_FILE_NAME
        && name != WORLD_FILE_NAME)
    {
      countries.push_back(&CountryLeafByCountryId(countryId));
    }
  }
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

  return countryId == m_latestDiffRequest;
}

void Storage::SetLocale(string const & locale) { m_countryNameGetter.SetLocale(locale); }
string Storage::GetLocale() const { return m_countryNameGetter.GetLocale(); }
void Storage::SetDownloaderForTesting(unique_ptr<MapFilesDownloader> downloader)
{
  m_downloader = move(downloader);
}

void Storage::SetEnabledIntegrityValidationForTesting(bool enabled)
{
  m_integrityValidationEnabled = enabled;
}

void Storage::SetCurrentDataVersionForTesting(int64_t currentVersion)
{
  m_currentVersion = currentVersion;
}

void Storage::SetDownloadingServersForTesting(vector<string> const & downloadingUrls)
{
  m_downloader->SetServersList(downloadingUrls);
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
  LocalFilePtr fakeCountryLocalFile = make_shared<LocalCountryFile>(localFile);
  fakeCountryLocalFile->SyncWithDisk();
  m_localFilesForFakeCountries[fakeCountryLocalFile->GetCountryFile()] = fakeCountryLocalFile;
}

void Storage::DeleteCountryFiles(CountryId const & countryId, MapFileType type, bool deferredDelete)
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
    DeleteFromDiskWithIndexes(*localFile, type);
    localFile->SyncWithDisk();
    if (!localFile->HasFiles())
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

  if (IsDiffApplyingInProgressToCountry(countryId))
  {
    m_diffsCancellable.Cancel();
    m_diffsBeingApplied.erase(countryId);
  }

  if (IsCountryFirstInQueue(countryId))
  {
    m_downloader->Reset();

    // Remove all files the downloader has created for the country.
    DeleteDownloaderFilesForCountry(GetCurrentDataVersion(), m_dataDir, GetCountryFile(countryId));
  }

  auto it = find(m_queue.begin(), m_queue.end(), countryId);
  ASSERT(it != m_queue.end(), ());
  PopFromQueue(it);
  SaveDownloadQueue();

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
  if (queuedCountry.GetFileType() == MapFileType::Diff)
  {
    CHECK(m_diffManager.SizeToDownloadFor(countryId, size), ());
    return size;
  }

  CountryFile const & file = GetCountryFile(countryId);
  return GetRemoteSize(file, GetCurrentDataVersion());
}

string Storage::GetFileDownloadPath(CountryId const & countryId, MapFileType type) const
{
  return platform::GetFileDownloadPath(GetCurrentDataVersion(), m_dataDir,
                                       GetCountryFile(countryId), type);
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

  CountryTree::Node const * const parentNode = m_countries.FindFirst(parent);
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

  CountryTree::Node const * const parentNode = m_countries.FindFirst(parent);
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
  parentNode->ForEachChild([&](CountryTree::Node const & childNode) {
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

  LOG(LINFO, ("Downloading", countryId));

  CountryTree::Node const * const node = m_countries.FindFirst(countryId);

  if (!node)
    return;

  if (GetNodeStatus(*node).status == NodeStatus::OnDisk)
    return;

  auto downloadAction = [this, isUpdate](CountryTree::Node const & descendantNode) {
    if (descendantNode.ChildrenCount() == 0 &&
        GetNodeStatus(descendantNode).status != NodeStatus::OnDisk)
    {
      DownloadCountry(descendantNode.Value().Name(),
                      isUpdate ? MapFileType::Diff : MapFileType::Map);
    }
  };

  node->ForEachInSubtree(downloadAction);
}

void Storage::DeleteNode(CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryTree::Node const * const node = m_countries.FindFirst(countryId);

  if (!node)
    return;

  auto deleteAction = [this](CountryTree::Node const & descendantNode) {
    bool onDisk = m_localFiles.find(descendantNode.Value().Name()) != m_localFiles.end();
    if (descendantNode.ChildrenCount() == 0 && onDisk)
      this->DeleteCountry(descendantNode.Value().Name(), MapFileType::Map);
  };
  node->ForEachInSubtree(deleteAction);
}

StatusAndError Storage::GetNodeStatus(CountryTree::Node const & node) const
{
  vector<pair<CountryId, NodeStatus>> disputedTerritories;
  return GetNodeStatusInfo(node, disputedTerritories, false /* isDisputedTerritoriesCounted */);
}

bool Storage::IsDisputed(CountryTree::Node const & node) const
{
  vector<CountryTree::Node const *> found;
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

  diffs::Loader::Load(move(localMapsInfo), [this](diffs::NameDiffInfoMap && diffs)
  {
    OnDiffStatusReceived(move(diffs));
  });
}

void Storage::ApplyDiff(CountryId const & countryId, function<void(bool isSuccess)> const & fn)
{
  LOG(LINFO, ("Applying diff for", countryId));

  m_diffsCancellable.Reset();
  auto const diffLocalFile = PreparePlaceForCountryFiles(GetCurrentDataVersion(), m_dataDir,
                                                         GetCountryFile(countryId));
  uint64_t version;
  if (!diffLocalFile || !m_diffManager.VersionFor(countryId, version))
  {
    fn(false);
    return;
  }

  m_latestDiffRequest = countryId;
  m_diffsBeingApplied.insert(countryId);
  NotifyStatusChangedForHierarchy(countryId);

  diffs::Manager::ApplyDiffParams params;
  params.m_diffFile = diffLocalFile;
  params.m_diffReadyPath = GetFileDownloadPath(countryId, MapFileType::Diff);
  params.m_oldMwmFile = GetLocalFile(countryId, version);

  LocalFilePtr & diffFile = params.m_diffFile;

  diffs::Manager::ApplyDiff(
      move(params), m_diffsCancellable,
      [this, fn, countryId, diffFile](DiffApplicationResult result) {
        CHECK_THREAD_CHECKER(m_threadChecker, ());

        static string const kSourceKey = "diff";
        if (result == DiffApplicationResult::Ok && m_integrityValidationEnabled &&
            !ValidateIntegrity(diffFile, diffFile->GetCountryName(), kSourceKey))
        {
          GetPlatform().RunTask(Platform::Thread::File,
            [path = diffFile->GetPath(MapFileType::Map)] { base::DeleteFileX(path); });
          result = DiffApplicationResult::Failed;
        }

        if (m_diffsBeingApplied.count(countryId) == 0 && result == DiffApplicationResult::Ok)
          result = DiffApplicationResult::Cancelled;

        LOG(LINFO, ("Diff application result for", countryId, ":", result));

        m_latestDiffRequest = {};
        m_diffsBeingApplied.erase(countryId);
        switch (result)
        {
        case DiffApplicationResult::Ok:
        {
          RegisterCountryFiles(diffFile);
          Platform::DisableBackupForFile(diffFile->GetPath(MapFileType::Map));
          m_diffManager.MarkAsApplied(countryId);
          fn(true);
          break;
        }
        case DiffApplicationResult::Cancelled:
        {
          if (m_downloader->IsIdle())
            DownloadNextCountryFromQueue();
          break;
        }
        case DiffApplicationResult::Failed:
        {
          m_diffManager.RemoveDiffForCountry(countryId);
          fn(false);
          break;
        }
        }
      });
}

bool Storage::IsPossibleToAutoupdate() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (m_diffManager.GetStatus() != diffs::Status::Available)
    return false;

  auto const currentVersion = GetCurrentDataVersion();
  CountriesVec localMaps;
  GetLocalRealMaps(localMaps);
  for (auto const & countryId : localMaps)
  {
    auto const localFile = GetLatestLocalFile(countryId);
    auto const mapVersion = localFile->GetVersion();
    if (mapVersion != currentVersion && mapVersion > 0 &&
        !m_diffManager.HasDiffFor(localFile->GetCountryName()))
    {
      return false;
    }
  }

  return true;
}

void Storage::SetStartDownloadingCallback(StartDownloadingCallback const & cb)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_startDownloadingCallback = cb;
}

void Storage::OnDiffStatusReceived(diffs::NameDiffInfoMap && diffs)
{
  m_downloader->SetDiffs(diffs);
  m_diffManager.Load(move(diffs));
  if (m_diffManager.GetStatus() != diffs::Status::NotAvailable)
  {
    for (auto const & localDiff : m_notAppliedDiffs)
    {
      auto const countryId = FindCountryIdByFile(localDiff.GetCountryName());

      if (m_diffManager.HasDiffFor(countryId))
        UpdateNode(countryId);
      else
        localDiff.DeleteFromDisk(MapFileType::Diff);
    }

    m_notAppliedDiffs.clear();
  }

  DoDeferredDownloadIfNeeded();
}

StatusAndError Storage::GetNodeStatusInfo(CountryTree::Node const & node,
                                          vector<pair<CountryId, NodeStatus>> & disputedTerritories,
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

  auto groupStatusCalculator = [&](CountryTree::Node const & nodeInSubtree) {
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

  vector<CountryTree::Node const *> nodes;
  m_countries.Find(countryId, nodes);
  CHECK(!nodes.empty(), (countryId));
  // If nodes.size() > 1 countryId corresponds to a disputed territories.
  // In that case it's guaranteed that most of attributes are equal for
  // each element of nodes. See Country class description for further details.
  CountryTree::Node const * const node = nodes[0];

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
    MwmSize const subTreeSizeBytes = node->Value().GetSubtreeMwmSizeBytes();
    nodeAttrs.m_downloadingProgress.first = subTreeSizeBytes;
    nodeAttrs.m_downloadingProgress.second = subTreeSizeBytes;
  }
  else
  {
    CountriesVec subtree;
    node->ForEachInSubtree(
        [&subtree](CountryTree::Node const & d) { subtree.push_back(d.Value().Name()); });
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
  node->ForEachInSubtree([this, &nodeAttrs, &visitedLocalNodes](CountryTree::Node const & d) {
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
    nodeAttrs.m_localMwmSize += localFile->GetSize(MapFileType::Map);
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
      nodes, [&](CountryId const & ancestorId, CountryTree::Node const & node) {
        if (node.Value().GetParent() == GetRootId())
          nodeAttrs.m_topmostParentInfo.push_back({ancestorId, m_countryNameGetter(ancestorId)});
      });
}

void Storage::GetNodeStatuses(CountryId const & countryId, NodeStatuses & nodeStatuses) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryTree::Node const * const node = m_countries.FindFirst(countryId);
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
      localAndRemoteBytes.second += GetRemoteSize(GetCountryFile(d), GetCurrentDataVersion());
    }
    else if (mwmsInQueue.count(d) != 0)
    {
      localAndRemoteBytes.second += GetRemoteSize(GetCountryFile(d), GetCurrentDataVersion());
    }
    else if (m_justDownloaded.count(d) != 0)
    {
      MwmSize const localCountryFileSz = GetRemoteSize(GetCountryFile(d), GetCurrentDataVersion());
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

  LOG(LINFO, ("Cancelling the downloading of", countryId));

  CountriesSet setQueue;
  GetQueuedCountries(m_queue, setQueue);

  ForEachInSubtree(countryId, [&](CountryId const & descendantId, bool /* groupNode */) {
    auto needNotify = false;
    if (setQueue.count(descendantId) != 0)
      needNotify = DeleteCountryFilesFromDownloader(descendantId);

    if (m_failedCountries.count(descendantId) != 0)
    {
      m_failedCountries.erase(descendantId);
      needNotify = true;
    }

    if (needNotify)
      NotifyStatusChangedForHierarchy(countryId);
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
  auto const updateInfoAccumulator = [&updateInfo, this](CountryTree::Node const & node) {
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

    LocalAndRemoteSize sizes = CountrySizeInBytes(node.Value().Name());
    updateInfo.m_sizeDifference +=
        static_cast<int64_t>(sizes.second) - static_cast<int64_t>(sizes.first);
  };

  CountryTree::Node const * const node = m_countries.FindFirst(countryId);
  if (!node)
  {
    ASSERT(false, ());
    return false;
  }
  updateInfo = UpdateInfo();
  node->ForEachInSubtree(updateInfoAccumulator);
  return true;
}

std::vector<base::GeoObjectId> Storage::GetTopCountryGeoIds(CountryId const & countryId) const
{
  std::vector<base::GeoObjectId> result;

  auto const collector = [this, &result](CountryId const & id, CountryTree::Node const &) {
    auto const it = m_mwmTopCountryGeoIds.find(id);
    if (it != m_mwmTopCountryGeoIds.cend())
      result.insert(result.end(), it->second.cbegin(), it->second.cend());
  };

  ForEachAncestorExceptForTheRoot(countryId, collector);

  return result;
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
  CountryTree::Node const * const node = m_countries.FindFirst(parent);
  if (!node)
  {
    ASSERT(false, ());
    return;
  }

  queuedChildren.clear();
  node->ForEachChild([&queuedChildren, this](CountryTree::Node const & child) {
    NodeStatus status = GetNodeStatus(child).status;
    ASSERT_NOT_EQUAL(status, NodeStatus::Undefined, ());
    if (status == NodeStatus::Downloading || status == NodeStatus::InQueue)
      queuedChildren.push_back(child.Value().Name());
  });
}

void Storage::GetGroupNodePathToRoot(CountryId const & groupNode, CountriesVec & path) const
{
  path.clear();

  vector<CountryTree::Node const *> nodes;
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
      nodes, [&path](CountryId const & id, CountryTree::Node const &) { path.push_back(id); });
  path.push_back(m_countries.GetRoot().Value().Name());
}

void Storage::GetTopmostNodesFor(CountryId const & countryId, CountriesVec & nodes,
                                 size_t level) const
{
  nodes.clear();

  vector<CountryTree::Node const *> treeNodes;
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
        [&path](CountryId const & id, CountryTree::Node const &) { path.emplace_back(id); });
    if (!path.empty() && level < path.size())
      nodes[i] = path[path.size() - 1 - level];
  }
}

CountryId const Storage::GetParentIdFor(CountryId const & countryId) const
{
  vector<CountryTree::Node const *> nodes;
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

CountryId const Storage::GetTopmostParentFor(CountryId const & countryId) const
{
  auto const rootId = GetRootId();
  auto result = countryId;
  auto parent = GetParentIdFor(result);
  while (!parent.empty() && parent != rootId)
  {
    result = move(parent);
    parent = GetParentIdFor(result);
  }

  return result;
}

MwmSize Storage::GetRemoteSize(CountryFile const & file, int64_t version) const
{
  uint64_t size;
  if (m_diffManager.SizeFor(file.GetName(), size))
    return size;
  return file.GetRemoteSize();

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
