#include "storage/http_map_files_downloader.hpp"
#include "storage/storage.hpp"

#include "defines.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/servers_list.hpp"
#include "platform/settings.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"
#include "coding/url_encode.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/sstream.hpp"
#include "std/target_os.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

using namespace downloader;
using namespace platform;

namespace storage
{
namespace
{
template <typename T>
void RemoveIf(vector<T> & v, function<bool(T const & t)> const & p)
{
  v.erase(remove_if(v.begin(), v.end(), p), v.end());
}

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

TCountriesContainer const & LeafNodeFromCountryId(TCountriesContainer const & root,
                                                  TCountryId const & countryId)
{
  CountryTree<Country> const * node = root.FindFirstLeaf(Country(countryId));
  CHECK(node, ("Node with id =", countryId, "not found in country tree as a leaf."));
  return *node;
}

void GetQueuedCountries(Storage::TQueue const & queue, TCountriesSet & countries)
{
  for (auto const & country : queue)
    countries.insert(country.GetCountryId());
}

void CorrectJustDownloaded(Storage::TQueue::iterator justDownloadedItem, Storage::TQueue & queue,
                           TCountriesSet & justDownloaded)
{
  TCountryId const justDownloadedCountry = justDownloadedItem->GetCountryId();
  queue.erase(justDownloadedItem);
  if (queue.empty())
    justDownloaded.clear();
  else
    justDownloaded.insert(justDownloadedCountry);
}

bool IsDownloadingStatus(Status status)
{
  return status == Status::EDownloading || status == Status::EInQueue;
}

bool IsUserAttentionNeededStatus(Status status)
{
  return status == Status::EDownloadFailed || status == Status::EOutOfMemFailed
      || status == Status::EOnDiskOutOfDate || status == Status::EUnknown;
}
}  // namespace

bool HasCountryId(TCountriesVec const & sortedCountryIds, TCountryId const & countryId)
{
  ASSERT(is_sorted(sortedCountryIds.begin(), sortedCountryIds.end()), ());
  return binary_search(sortedCountryIds.begin(), sortedCountryIds.end(), countryId);
}

Storage::Storage(string const & pathToCountriesFile /* = COUNTRIES_FILE */, string const & dataDir /* = string() */)
  : m_downloader(new HttpMapFilesDownloader()), m_currentSlotId(0), m_dataDir(dataDir),
    m_downloadMapOnTheMap(nullptr)
{
  LoadCountriesFile(pathToCountriesFile, m_dataDir);
}

Storage::Storage(string const & referenceCountriesTxtJsonForTesting,
                 unique_ptr<MapFilesDownloader> mapDownloaderForTesting)
  : m_downloader(move(mapDownloaderForTesting)), m_currentSlotId(0),
    m_downloadMapOnTheMap(nullptr)
{
  m_currentVersion = LoadCountries(referenceCountriesTxtJsonForTesting, m_countries);
  CHECK_LESS_OR_EQUAL(0, m_currentVersion, ("Can't load test countries file"));
}

void Storage::Init(TUpdate const & update)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  m_update = update;
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

  m_prefetchStorage.reset(new Storage(COUNTRIES_MIGRATE_FILE, "migrate"));
  m_prefetchStorage->Init([](LocalCountryFile const &){});
  if (!m_downloadingUrlsForTesting.empty())
    m_prefetchStorage->SetDownloadingUrlsForTesting(m_downloadingUrlsForTesting);
}

void Storage::Migrate(TCountriesVec const & existedCountries)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  platform::migrate::SetMigrationFlag();

  Clear();
  m_countries.Clear();

  TMapping mapping;
  LoadCountriesFile(COUNTRIES_MIGRATE_FILE, m_dataDir, &mapping);

  vector<TCountryId> prefetchedMaps;
  m_prefetchStorage->GetLocalRealMaps(prefetchedMaps);

  // Move prefetched maps into current storage.
  for (auto const & countryId : prefetchedMaps)
  {
    string prefetchedFilename = m_prefetchStorage->GetLatestLocalFile(countryId)->GetPath(MapOptions::Map);
    CountryFile const countryFile = GetCountryFile(countryId);
    auto localFile = PreparePlaceForCountryFiles(GetCurrentDataVersion(), m_dataDir, countryFile);
    string localFilename = localFile->GetPath(MapOptions::Map);
    LOG_SHORT(LINFO, ("Move", prefetchedFilename, "to", localFilename));
    my::RenameFileX(prefetchedFilename, localFilename);
  }

  // Cover old big maps with small ones and prepare them to add into download queue
  stringstream ss;
  for (auto const & country : existedCountries)
  {
    ASSERT(!mapping[country].empty(), ());
    for (auto const & smallCountry : mapping[country])
      ss << (ss.str().empty() ? "" : ";") << smallCountry;
  }
  Settings::Set("DownloadQueue", ss.str());
}

void Storage::Clear()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  m_downloader->Reset();
  m_queue.clear();
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

  auto compareByCountryAndVersion = [](LocalCountryFile const & lhs, LocalCountryFile const & rhs)
  {
    if (lhs.GetCountryFile() != rhs.GetCountryFile())
      return lhs.GetCountryFile() < rhs.GetCountryFile();
    return lhs.GetVersion() > rhs.GetVersion();
  };

  auto equalByCountry = [](LocalCountryFile const & lhs, LocalCountryFile const & rhs)
  {
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
    if (IsCountryIdValid(countryId) && IsCoutryIdInCountryTree(countryId))
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
  CountryTree<Country> const * node = m_countries.FindFirst(Country(countryId));
  CHECK(node, ("Node with id =", countryId, "not found in country tree."));
  return node->Value();
}

bool Storage::IsCoutryIdInCountryTree(TCountryId const & countryId) const
{
  return m_countries.FindFirst(Country(countryId)) != nullptr;
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
    sizes.first = m_downloader->GetDownloadingProgress().first +
                  GetRemoteSize(countryFile, queuedCountry->GetDownloadedFiles(),
                                GetCurrentDataVersion());
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
  if (IsCountryIdValid(countryId) && IsCoutryIdInCountryTree(countryId))
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
  // Check if we already downloading this country or have it in the queue
  if (IsCountryInQueue(countryId))
  {
    if (IsCountryFirstInQueue(countryId))
      return Status::EDownloading;
    else
      return Status::EInQueue;
  }

  // Check if this country has failed while downloading.
  if (m_failedCountries.count(countryId) > 0)
    return Status::EDownloadFailed;

  return Status::EUnknown;
}

Status Storage::CountryStatusEx(TCountryId const & countryId) const
{
  return CountryStatusFull(countryId, CountryStatus(countryId));
}

void Storage::CountryStatusEx(TCountryId const & countryId, Status & status, MapOptions & options) const
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

  stringstream ss;
  for (auto const & item : m_queue)
    ss << (ss.str().empty() ? "" : ";") << item.GetCountryId();
  Settings::Set("DownloadQueue", ss.str());
}

void Storage::RestoreDownloadQueue()
{
  string queue;
  if (!Settings::Get("DownloadQueue", queue))
    return;

  strings::SimpleTokenizer iter(queue, ";");
  while (iter)
  {
    DownloadCountry(*iter, MapOptions::MapWithCarRouting);
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
    NotifyStatusChanged(countryId);
  SaveDownloadQueue();
}

void Storage::DeleteCountry(TCountryId const & countryId, MapOptions opt)
{
  ASSERT(m_update != nullptr, ("Storage::Init wasn't called"));

  opt = NormalizeDeleteFileSet(opt);
  DeleteCountryFiles(countryId, opt);
  DeleteCountryFilesFromDownloader(countryId, opt);

  TLocalFilePtr localFile = GetLatestLocalFile(countryId);
  if (localFile)
    m_update(*localFile);

  NotifyStatusChanged(countryId);
}

void Storage::DeleteCustomCountryVersion(LocalCountryFile const & localFile)
{
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
  if (!(IsCountryIdValid(countryId) && IsCoutryIdInCountryTree(countryId)))
  {
    LOG(LERROR, ("Removed files for an unknown country:", localFile));
    return;
  }

  MY_SCOPE_GUARD(notifyStatusChanged, bind(&Storage::NotifyStatusChanged, this, countryId));

  // If file version equals to current data version, delete from downloader all pending requests for
  // the country.
  if (localFile.GetVersion() == GetCurrentDataVersion())
    DeleteCountryFilesFromDownloader(countryId, MapOptions::MapWithCarRouting);
  auto countryFilesIt = m_localFiles.find(countryId);
  if (countryFilesIt == m_localFiles.end())
  {
    LOG(LERROR, ("Deleted files of an unregistered country:", localFile));
    return;
  }

  auto const equalsToLocalFile = [&localFile](TLocalFilePtr const & rhs)
  {
    return localFile == *rhs;
  };
  countryFilesIt->second.remove_if(equalsToLocalFile);
}

void Storage::NotifyStatusChanged(TCountryId const & countryId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  for (CountryObservers const & observer : m_observers)
    observer.m_changeCountryFn(countryId);
}

void Storage::DownloadNextCountryFromQueue()
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  if (m_queue.empty())
    return;

  QueuedCountry & queuedCountry = m_queue.front();
  TCountryId const & countryId = queuedCountry.GetCountryId();

  // It's not even possible to prepare directory for files before
  // downloading.  Mark this country as failed and switch to next
  // country.
  if (!PreparePlaceForCountryFiles(GetCurrentDataVersion(), m_dataDir, GetCountryFile(countryId)))
  {
    OnMapDownloadFinished(countryId, false /* success */, queuedCountry.GetInitOptions());
    NotifyStatusChanged(countryId);
    CorrectJustDownloaded(m_queue.begin(), m_queue, m_justDownloaded);
    DownloadNextCountryFromQueue();
    return;
  }

  DownloadNextFile(queuedCountry);

  // New status for the country, "Downloading"
  NotifyStatusChanged(queuedCountry.GetCountryId());
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

bool Storage::DeleteFromDownloader(TCountryId const & countryId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  if (!DeleteCountryFilesFromDownloader(countryId, MapOptions::MapWithCarRouting))
    return false;
  NotifyStatusChanged(countryId);
  return true;
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

void Storage::LoadCountriesFile(string const & pathToCountriesFile,
                                string const & dataDir, TMapping * mapping /* = nullptr */)
{
  m_dataDir = dataDir;

  if (!m_dataDir.empty())
  {
    Platform & platform = GetPlatform();
    platform.MkDir(my::JoinFoldersToPath(platform.WritableDir(), m_dataDir));
  }

  if (m_countries.ChildrenCount() == 0)
  {
    string json;
    ReaderPtr<Reader>(GetPlatform().GetReader(pathToCountriesFile)).ReadAsString(json);
    m_currentVersion = LoadCountries(json, m_countries, mapping);
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
  CorrectJustDownloaded(m_queue.begin(), m_queue, m_justDownloaded);
  SaveDownloadQueue();

  NotifyStatusChanged(countryId);
  m_downloader->Reset();
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

  auto calcProgress = [&]
      (TCountryId const & parentId, TCountriesContainer const & parentNode)
  {
    TCountriesVec descendants;
    parentNode.ForEachDescendant([&descendants](TCountriesContainer const & container)
    {
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
  ASSERT(m_update != nullptr, ("Storage::Init wasn't called"));
  ASSERT_NOT_EQUAL(MapOptions::Nothing, files,
                   ("This method should not be called for empty files set."));
  {
    alohalytics::LogEvent("$OnMapDownloadFinished",
        alohalytics::TStringMap({{"name", GetCountryFile(countryId).GetName()},
                                 {"status", success ? "ok" : "failed"},
                                 {"version", strings::to_string(GetCurrentDataVersion())},
                                 {"option", DebugPrint(files)}}));
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
  m_update(*localFile);
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
  if (m_countries.FindFirst(Country(name)))
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
    if (file && file->GetVersion() != GetCurrentDataVersion() &&
        name != WORLD_COASTS_FILE_NAME && name != WORLD_COASTS_MIGRATE_FILE_NAME && name != WORLD_FILE_NAME)
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

void Storage::SetLocale(string const & locale)
{
  m_countryNameGetter.SetLocale(locale);
}

string Storage::GetLocale() const
{
  return m_countryNameGetter.GetLocale();
}

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

void Storage::RegisterCountryFiles(TCountryId const & countryId, string const & directory, int64_t version)
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
  TLocalFilePtr fakeCountryLocalFile = make_shared<LocalCountryFile>(localFile);
  fakeCountryLocalFile->SyncWithDisk();
  m_localFilesForFakeCountries[fakeCountryLocalFile->GetCountryFile()] = fakeCountryLocalFile;
}

void Storage::DeleteCountryFiles(TCountryId const & countryId, MapOptions opt)
{
  auto const it = m_localFiles.find(countryId);
  if (it == m_localFiles.end())
    return;

  auto & localFiles = it->second;
  for (auto & localFile : localFiles)
  {
    DeleteFromDiskWithIndexes(*localFile, opt);
    localFile->SyncWithDisk();
    if (localFile->GetFiles() == MapOptions::Nothing)
      localFile.reset();
  }
  auto isNull = [](TLocalFilePtr const & localFile)
  {
    return !localFile;
  };
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
    CorrectJustDownloaded(find(m_queue.begin(), m_queue.end(), countryId), m_queue, m_justDownloaded);

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
  return platform::GetFileDownloadPath(GetCurrentDataVersion(), m_dataDir, GetCountryFile(countryId), file);
}

TCountryId const Storage::GetRootId() const
{
  return m_countries.Value().Name();
}

void Storage::GetChildren(TCountryId const & parent, TCountriesVec & childrenId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  TCountriesContainer const * const parentNode = m_countries.FindFirst(Country(parent));
  if (parentNode == nullptr)
  {
    ASSERT(false, ("TCountryId =", parent, "not found in m_countries."));
    return;
  }

  size_t const childrenCount = parentNode->ChildrenCount();
  childrenId.clear();
  childrenId.reserve(childrenCount);
  for (size_t i = 0; i < childrenCount; ++i)
    childrenId.emplace_back(parentNode->Child(i).Value().Name());
}

void Storage::GetLocalRealMaps(TCountriesVec & localMaps) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  localMaps.clear();
  localMaps.reserve(m_localFiles.size());

  for(auto const & keyValue : m_localFiles)
    localMaps.push_back(keyValue.first);
}

void Storage::GetDownloadedChildren(TCountryId const & parent, TCountriesVec & localChildren) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  TCountriesContainer const * const parentNode = m_countries.FindFirst(Country(parent));
  if (parentNode == nullptr)
  {
    ASSERT(false, ("TCountryId =", parent, "not found in m_countries."));
    return;
  }

  localChildren.clear();
  TCountriesVec localMaps;
  GetLocalRealMaps(localMaps);
  if (localMaps.empty())
    return;

  size_t const childrenCount = parentNode->ChildrenCount();
  sort(localMaps.begin(), localMaps.end());

  for (size_t i = 0; i < childrenCount; ++i)
  {
    TCountriesContainer const & child = parentNode->Child(i);
    TCountryId const & childCountryId = child.Value().Name();
    if (HasCountryId(localMaps, childCountryId))
    { // CountryId of child is a name of an mwm.
      localChildren.push_back(childCountryId);
      continue;
    }

    // Child is a group of mwms.
    size_t localMapsInChild = 0;
    TCountryId lastCountryIdInLocalMaps;
    child.ForEachDescendant([&](TCountriesContainer const & descendant)
                            {
                              TCountryId const & countryId = descendant.Value().Name();
                              if (HasCountryId(localMaps, countryId))
                              {
                                ++localMapsInChild;
                                lastCountryIdInLocalMaps = countryId;
                              }
                            });
    if (localMapsInChild == 0)
      continue; // No descendant of the child is in localMaps.
    if (localMapsInChild == 1)
      localChildren.push_back(lastCountryIdInLocalMaps); // One descendant of the child is in localMaps.
    else
      localChildren.push_back(childCountryId); // Two or more descendants of the child is in localMaps.
  }
}

bool Storage::IsNodeDownloaded(TCountryId const & countryId) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  for(auto const & localeMap : m_localFiles)
  {
    if (countryId == localeMap.first)
      return true;
  }
  return false;
}

void Storage::GetCountyListToDownload(TCountriesVec & countryList) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  TCountriesVec countryIds;
  GetChildren(GetRootId(), countryIds);
  // @TODO(bykoianko) Implement this method. Remove from this method fully downloaded maps.
}

bool Storage::DownloadNode(TCountryId const & countryId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  // @TODO(bykoianko) Before downloading it's necessary to check if file(s) has been downloaded.
  // If so, the method should be left with false.
  TCountriesContainer const * const node = m_countries.FindFirst(Country(countryId));
  CHECK(node, ());
  node->ForEachInSubtree([this](TCountriesContainer const & descendantNode)
                         {
                           if (descendantNode.ChildrenCount() == 0)
                           {
                             this->DownloadCountry(descendantNode.Value().Name(),
                                                   MapOptions::MapWithCarRouting);
                           }
                         });
  return true;
}

void Storage::DeleteNode(TCountryId const & countryId)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  TCountriesContainer const * const node = m_countries.FindFirst(Country(countryId));

  if (!node)
    return;

  auto deleteAction = [this](TCountriesContainer const & descendantNode)
  {
    if (descendantNode.ChildrenCount() == 0)
      this->DeleteCountry(descendantNode.Value().Name(), MapOptions::MapWithCarRouting);
  };
  node->ForEachInSubtree(deleteAction);
}

Status Storage::NodeStatus(TCountriesContainer const & node) const
{
  // Leaf node status.
  if (node.ChildrenCount() == 0)
    return CountryStatusEx(node.Value().Name());

  // Group node status.
  Status const kDownloadingInProgress = Status::EDownloading;
  Status const kUsersAttentionNeeded =  Status::EDownloadFailed;
  Status const kEverythingOk = Status::EOnDisk;

  Status result = kEverythingOk;
  auto groupStatusCalculator = [&result, this](TCountriesContainer const & nodeInSubtree)
  {
    if (result == kDownloadingInProgress || nodeInSubtree.ChildrenCount() != 0)
      return;
    Status status = this->CountryStatusEx(nodeInSubtree.Value().Name());
    ASSERT_NOT_EQUAL(status, Status::EUndefined, ());

    if (IsDownloadingStatus(status))
    {
      result = kDownloadingInProgress;
      return;
    }

    if (result == kUsersAttentionNeeded)
      return;

    if (IsUserAttentionNeededStatus(status))
    {
      result = kUsersAttentionNeeded;
      return;
    }
  };

  node.ForEachDescendant(groupStatusCalculator);
  return result;
}

void Storage::GetNodeAttrs(TCountryId const & countryId, NodeAttrs & nodeAttrs) const
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ());

  vector<CountryTree<Country> const *> nodes;
  m_countries.Find(Country(countryId), nodes);
  CHECK(!nodes.empty(), ());
  // If nodes.size() > 1 countryId corresponds to a disputed territories.
  // In that case it's guaranteed that most of attributes are equal for
  // each element of nodes. See Country class description for further details.
  TCountriesContainer const * const node = nodes[0];

  Country const & nodeValue = node->Value();
  nodeAttrs.m_mwmCounter = nodeValue.GetSubtreeMwmCounter();
  nodeAttrs.m_mwmSize = nodeValue.GetSubtreeMwmSizeBytes();
  StatusAndError statusAndErr = ParseStatus(NodeStatus(*node));
  nodeAttrs.m_status = statusAndErr.status;
  nodeAttrs.m_error = statusAndErr.error;
  nodeAttrs.m_nodeLocalName = m_countryNameGetter(countryId);

  TCountriesVec descendants;
  node->ForEachDescendant([&descendants](TCountriesContainer const & d)
  {
    descendants.push_back(d.Value().Name());
  });
  TCountryId const & downloadingMwm = IsDownloadInProgress() ? GetCurrentDownloadingCountryId()
                                                             : kInvalidCountryId;
  MapFilesDownloader::TProgress downloadingMwmProgress =
      m_downloader->IsIdle() ? make_pair(0LL, 0LL)
                             : m_downloader->GetDownloadingProgress();

  TCountriesSet setQueue;
  GetQueuedCountries(m_queue, setQueue);
  nodeAttrs.m_downloadingProgress =
      CalculateProgress(downloadingMwm, descendants, downloadingMwmProgress, setQueue);

  nodeAttrs.m_parentInfo.clear();
  nodeAttrs.m_parentInfo.reserve(nodes.size());
  for (auto const & n : nodes)
  {
    Country const & nValue = n->Value();
    CountryIdAndName countryIdAndName;
    countryIdAndName.m_id = nValue.GetParent();
    if (countryIdAndName.m_id.empty()) // The root case.
      countryIdAndName.m_localName = string();
    else
      countryIdAndName.m_localName = m_countryNameGetter(countryIdAndName.m_id);
    nodeAttrs.m_parentInfo.emplace_back(move(countryIdAndName));
  }
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

MapFilesDownloader::TProgress Storage::CalculateProgress(TCountryId const & downloadingMwm,
                                                         TCountriesVec const & mwms,
                                                         MapFilesDownloader::TProgress const & downloadingMwmProgress,
                                                         TCountriesSet const & mwmsInQueue) const
{
  MapFilesDownloader::TProgress localAndRemoteBytes = make_pair(0, 0);
  if (downloadingMwm != kInvalidCountryId)
    localAndRemoteBytes = downloadingMwmProgress;

  for (auto const & d : mwms)
  {
    if (d == downloadingMwm)
      continue;

    if (mwmsInQueue.count(d) != 0)
    {
      CountryFile const & remoteCountryFile = GetCountryFile(d);
      localAndRemoteBytes.second += remoteCountryFile.GetRemoteSize(MapOptions::Map);
      continue;
    }

    if (m_justDownloaded.count(d) != 0)
    {
      size_t const localCountryFileSz = GetCountryFile(d).GetRemoteSize(MapOptions::Map);
      localAndRemoteBytes.first += localCountryFileSz;
      localAndRemoteBytes.second += localCountryFileSz;
    }
  }

  return localAndRemoteBytes;
}
}  // namespace storage
