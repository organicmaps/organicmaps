#include "storage/storage.hpp"
#include "storage/http_map_files_downloader.hpp"

#include "defines.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"
#include "platform/servers_list.hpp"

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

uint64_t GetRemoteSize(CountryFile const & file, MapOptions opt)
{
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

class EqualFileName
{
  string const & m_name;

public:
  explicit EqualFileName(string const & name) : m_name(name) {}
  bool operator()(SimpleTree<Country> const & node) const
  {
    Country const & c = node.Value();
    if (c.GetFilesCount() > 0)
      return (c.GetFile().GetNameWithoutExt() == m_name);
    else
      return false;
  }
};
}  // namespace

Storage::Storage() : m_downloader(new HttpMapFilesDownloader()), m_currentSlotId(0)
{
  LoadCountriesFile(false /* forceReload */);
}

void Storage::Init(TUpdate const & update) { m_update = update; }

void Storage::Clear()
{
  m_downloader->Reset();
  m_queue.clear();
  m_failedCountries.clear();
  m_localFiles.clear();
  m_localFilesForFakeCountries.clear();
}

void Storage::RegisterAllLocalMaps()
{
  m_localFiles.clear();
  m_localFilesForFakeCountries.clear();

  vector<LocalCountryFile> localFiles;
  FindAllLocalMapsAndCleanup(GetCurrentDataVersion(), localFiles);

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
    TIndex index = FindIndexByFile(name);
    if (index.IsValid())
      RegisterCountryFiles(index, localFile.GetDirectory(), localFile.GetVersion());
    else
      RegisterFakeCountryFiles(localFile);

    LOG(LINFO, ("Found file:", name, "in directory:", localFile.GetDirectory()));

    i = j;
  }
}

void Storage::GetLocalMaps(vector<TLocalFilePtr> & maps) const
{
  for (auto const & p : m_localFiles)
    maps.push_back(GetLatestLocalFile(p.first));

  for (auto const & p : m_localFilesForFakeCountries)
    maps.push_back(p.second);

  maps.erase(unique(maps.begin(), maps.end()), maps.end());
}

size_t Storage::GetDownloadedFilesCount() const
{
  return m_localFiles.size();
}

CountriesContainerT const & NodeFromIndex(CountriesContainerT const & root, TIndex const & index)
{
  // complex logic to avoid [] out_of_bounds exceptions
  if (index.m_group == TIndex::INVALID || index.m_group >= static_cast<int>(root.SiblingsCount()))
    return root;
  if (index.m_country == TIndex::INVALID ||
      index.m_country >= static_cast<int>(root[index.m_group].SiblingsCount()))
  {
    return root[index.m_group];
  }
  if (index.m_region == TIndex::INVALID ||
      index.m_region >= static_cast<int>(root[index.m_group][index.m_country].SiblingsCount()))
  {
    return root[index.m_group][index.m_country];
  }
  return root[index.m_group][index.m_country][index.m_region];
}

Country const & Storage::CountryByIndex(TIndex const & index) const
{
  return NodeFromIndex(m_countries, index).Value();
}

void Storage::GetGroupAndCountry(TIndex const & index, string & group, string & country) const
{
  string fName = CountryByIndex(index).GetFile().GetNameWithoutExt();
  CountryInfo::FileName2FullName(fName);
  CountryInfo::FullName2GroupAndMap(fName, group, country);
}

size_t Storage::CountriesCount(TIndex const & index) const
{
  return NodeFromIndex(m_countries, index).SiblingsCount();
}

string const & Storage::CountryName(TIndex const & index) const
{
  return NodeFromIndex(m_countries, index).Value().Name();
}

string const & Storage::CountryFlag(TIndex const & index) const
{
  return NodeFromIndex(m_countries, index).Value().Flag();
}

LocalAndRemoteSizeT Storage::CountrySizeInBytes(TIndex const & index, MapOptions opt) const
{
  QueuedCountry const * queuedCountry = FindCountryInQueue(index);
  TLocalFilePtr localFile = GetLatestLocalFile(index);
  CountryFile const & countryFile = GetCountryFile(index);
  if (queuedCountry == nullptr)
  {
    return LocalAndRemoteSizeT(GetLocalSize(localFile, opt), GetRemoteSize(countryFile, opt));
  }

  LocalAndRemoteSizeT sizes(0, GetRemoteSize(countryFile, opt));
  if (!m_downloader->IsIdle() && IsCountryFirstInQueue(index))
  {
    sizes.first = m_downloader->GetDownloadingProgress().first +
                  GetRemoteSize(countryFile, queuedCountry->GetDownloadedFiles());
  }
  return sizes;
}

CountryFile const & Storage::GetCountryFile(TIndex const & index) const
{
  return CountryByIndex(index).GetFile();
}

Storage::TLocalFilePtr Storage::GetLatestLocalFile(CountryFile const & countryFile) const
{
  TIndex const index = FindIndexByFile(countryFile.GetNameWithoutExt());
  if (index.IsValid())
  {
    TLocalFilePtr localFile = GetLatestLocalFile(index);
    if (localFile)
      return localFile;
  }

  auto const it = m_localFilesForFakeCountries.find(countryFile);
  if (it != m_localFilesForFakeCountries.end())
    return it->second;

  return TLocalFilePtr();
}

Storage::TLocalFilePtr Storage::GetLatestLocalFile(TIndex const & index) const
{
  auto const it = m_localFiles.find(index);
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

TStatus Storage::CountryStatus(TIndex const & index) const
{
  // Check if we already downloading this country or have it in the queue
  if (IsCountryInQueue(index))
  {
    if (IsCountryFirstInQueue(index))
      return TStatus::EDownloading;
    else
      return TStatus::EInQueue;
  }

  // Check if this country has failed while downloading.
  if (m_failedCountries.count(index) > 0)
    return TStatus::EDownloadFailed;

  return TStatus::EUnknown;
}

TStatus Storage::CountryStatusEx(TIndex const & index) const
{
  return CountryStatusFull(index, CountryStatus(index));
}

void Storage::CountryStatusEx(TIndex const & index, TStatus & status, MapOptions & options) const
{
  status = CountryStatusEx(index);

  if (status == TStatus::EOnDisk || status == TStatus::EOnDiskOutOfDate)
  {
    options = MapOptions::Map;

    TLocalFilePtr localFile = GetLatestLocalFile(index);
    ASSERT(localFile, ("Invariant violation: local file out of sync with disk."));
    if (localFile->OnDisk(MapOptions::CarRouting))
      options = SetOptions(options, MapOptions::CarRouting);
  }
}

void Storage::DownloadCountry(TIndex const & index, MapOptions opt)
{
  opt = NormalizeDownloadFileSet(index, opt);
  if (opt == MapOptions::Nothing)
    return;

  if (QueuedCountry * queuedCountry = FindCountryInQueue(index))
  {
    queuedCountry->AddOptions(opt);
    return;
  }

  m_failedCountries.erase(index);
  m_queue.push_back(QueuedCountry(index, opt));
  if (m_queue.size() == 1)
    DownloadNextCountryFromQueue();
  else
    NotifyStatusChanged(index);
}

void Storage::DeleteCountry(TIndex const & index, MapOptions opt)
{
  opt = NormalizeDeleteFileSet(opt);
  DeleteCountryFiles(index, opt);
  DeleteCountryFilesFromDownloader(index, opt);

  TLocalFilePtr localFile = GetLatestLocalFile(index);
  if (localFile)
    m_update(*localFile);

  NotifyStatusChanged(index);
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

  TIndex const index = FindIndexByFile(countryFile.GetNameWithoutExt());
  if (!index.IsValid())
  {
    LOG(LERROR, ("Removed files for an unknown country:", localFile));
    return;
  }

  MY_SCOPE_GUARD(notifyStatusChanged, bind(&Storage::NotifyStatusChanged, this, index));

  // If file version equals to current data version, delete from downloader all pending requests for
  // the country.
  if (localFile.GetVersion() == GetCurrentDataVersion())
    DeleteCountryFilesFromDownloader(index, MapOptions::MapWithCarRouting);
  auto countryFilesIt = m_localFiles.find(index);
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

void Storage::NotifyStatusChanged(TIndex const & index)
{
  for (CountryObservers const & observer : m_observers)
    observer.m_changeCountryFn(index);
}

void Storage::DownloadNextCountryFromQueue()
{
  if (m_queue.empty())
    return;

  QueuedCountry & queuedCountry = m_queue.front();
  TIndex const & index = queuedCountry.GetIndex();

  // It's not even possible to prepare directory for files before
  // downloading.  Mark this country as failed and switch to next
  // country.
  if (!PreparePlaceForCountryFiles(GetCountryFile(index), GetCurrentDataVersion()))
  {
    OnMapDownloadFinished(index, false /* success */, queuedCountry.GetInitOptions());
    NotifyStatusChanged(index);
    m_queue.pop_front();
    DownloadNextCountryFromQueue();
    return;
  }

  DownloadNextFile(queuedCountry);

  // New status for the country, "Downloading"
  NotifyStatusChanged(queuedCountry.GetIndex());
}

void Storage::DownloadNextFile(QueuedCountry const & country)
{
  TIndex const & index = country.GetIndex();
  CountryFile const & countryFile = GetCountryFile(index);

  string const filePath = GetFileDownloadPath(index, country.GetCurrentFile());
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
  m_downloader->GetServersList(GetCurrentDataVersion(), countryFile.GetNameWithoutExt(),
                               bind(&Storage::OnServerListDownloaded, this, _1));
}

bool Storage::DeleteFromDownloader(TIndex const & index)
{
  if (!DeleteCountryFilesFromDownloader(index, MapOptions::MapWithCarRouting))
    return false;
  NotifyStatusChanged(index);
  return true;
}

bool Storage::IsDownloadInProgress() const { return !m_queue.empty(); }

TIndex Storage::GetCurrentDownloadingCountryIndex() const { return IsDownloadInProgress() ? m_queue.front().GetIndex() : storage::TIndex(); }

void Storage::LoadCountriesFile(bool forceReload)
{
  if (forceReload)
    m_countries.Clear();

  if (m_countries.SiblingsCount() == 0)
  {
    string json;
    ReaderPtr<Reader>(GetPlatform().GetReader(COUNTRIES_FILE)).ReadAsString(json);
    m_currentVersion = LoadCountries(json, m_countries);
    if (m_currentVersion < 0)
      LOG(LERROR, ("Can't load countries file", COUNTRIES_FILE));
  }
}

int Storage::Subscribe(TChangeCountryFunction const & change, TProgressFunction const & progress)
{
  CountryObservers obs;

  obs.m_changeCountryFn = change;
  obs.m_progressFn = progress;
  obs.m_slotId = ++m_currentSlotId;

  m_observers.push_back(obs);

  return obs.m_slotId;
}

void Storage::Unsubscribe(int slotId)
{
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
  if (m_queue.empty())
    return;

  QueuedCountry & queuedCountry = m_queue.front();
  TIndex const index = queuedCountry.GetIndex();

  if (success && queuedCountry.SwitchToNextFile())
  {
    DownloadNextFile(queuedCountry);
    return;
  }

  OnMapDownloadFinished(index, success, queuedCountry.GetInitOptions());
  m_queue.pop_front();

  NotifyStatusChanged(index);
  m_downloader->Reset();
  DownloadNextCountryFromQueue();
}

void Storage::ReportProgress(TIndex const & idx, pair<int64_t, int64_t> const & p)
{
  for (CountryObservers const & o : m_observers)
    o.m_progressFn(idx, p);
}

void Storage::OnServerListDownloaded(vector<string> const & urls)
{
  // Queue can be empty because countries were deleted from queue.
  if (m_queue.empty())
    return;

  QueuedCountry const & queuedCountry = m_queue.front();
  TIndex const & index = queuedCountry.GetIndex();
  MapOptions const file = queuedCountry.GetCurrentFile();

  vector<string> fileUrls;
  fileUrls.reserve(urls.size());
  for (string const & url : urls)
    fileUrls.push_back(GetFileDownloadUrl(url, index, file));

  string const filePath = GetFileDownloadPath(index, file);
  m_downloader->DownloadMapFile(fileUrls, filePath, GetDownloadSize(queuedCountry),
                                bind(&Storage::OnMapFileDownloadFinished, this, _1, _2),
                                bind(&Storage::OnMapFileDownloadProgress, this, _1));
}

void Storage::OnMapFileDownloadProgress(MapFilesDownloader::TProgress const & progress)
{
  // Queue can be empty because countries were deleted from queue.
  if (m_queue.empty())
    return;

  if (!m_observers.empty())
  {
    QueuedCountry & queuedCountry = m_queue.front();
    CountryFile const & countryFile = GetCountryFile(queuedCountry.GetIndex());
    MapFilesDownloader::TProgress p = progress;
    p.first += GetRemoteSize(countryFile, queuedCountry.GetDownloadedFiles());
    p.second = GetRemoteSize(countryFile, queuedCountry.GetInitOptions());

    ReportProgress(m_queue.front().GetIndex(), p);
  }
}

bool Storage::RegisterDownloadedFiles(TIndex const & index, MapOptions files)
{
  CountryFile const countryFile = GetCountryFile(index);
  TLocalFilePtr localFile = GetLocalFile(index, GetCurrentDataVersion());
  if (!localFile)
    localFile = PreparePlaceForCountryFiles(countryFile, GetCurrentDataVersion());
  if (!localFile)
  {
    LOG(LERROR, ("Local file data structure can't be prepared for downloaded file(", countryFile,
                 files, ")."));
    return false;
  }

  bool ok = true;
  for (MapOptions file : {MapOptions::Map, MapOptions::CarRouting})
  {
    if (!HasOptions(files, file))
      continue;
    string const path = GetFileDownloadPath(index, file);
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

void Storage::OnMapDownloadFinished(TIndex const & index, bool success, MapOptions files)
{
  ASSERT_NOT_EQUAL(MapOptions::Nothing, files,
                   ("This method should not be called for empty files set."));
  {
    alohalytics::LogEvent("$OnMapDownloadFinished",
        alohalytics::TStringMap({{"name", GetCountryFile(index).GetNameWithoutExt()},
                                 {"status", success ? "ok" : "failed"},
                                 {"version", strings::to_string(GetCurrentDataVersion())},
                                 {"option", DebugPrint(files)}}));
  }

  success = success && RegisterDownloadedFiles(index, files);

  if (!success)
  {
    m_failedCountries.insert(index);
    return;
  }

  TLocalFilePtr localFile = GetLocalFile(index, GetCurrentDataVersion());
  ASSERT(localFile, ());
  DeleteCountryIndexes(*localFile);
  m_update(*localFile);
}

string Storage::GetFileDownloadUrl(string const & baseUrl, TIndex const & index,
                                   MapOptions file) const
{
  CountryFile const & countryFile = GetCountryFile(index);
  return GetFileDownloadUrl(baseUrl, countryFile.GetNameWithExt(file));
}

string Storage::GetFileDownloadUrl(string const & baseUrl, string const & fName) const
{
  return baseUrl + OMIM_OS_NAME "/" + strings::to_string(GetCurrentDataVersion()) + "/" +
         UrlEncode(fName);
}

TIndex Storage::FindIndexByFile(string const & name) const
{
  EqualFileName fn(name);

  for (size_t i = 0; i < m_countries.SiblingsCount(); ++i)
  {
    if (fn(m_countries[i]))
      return TIndex(static_cast<int>(i));

    for (size_t j = 0; j < m_countries[i].SiblingsCount(); ++j)
    {
      if (fn(m_countries[i][j]))
        return TIndex(static_cast<int>(i), static_cast<int>(j));

      for (size_t k = 0; k < m_countries[i][j].SiblingsCount(); ++k)
      {
        if (fn(m_countries[i][j][k]))
          return TIndex(static_cast<int>(i), static_cast<int>(j), static_cast<int>(k));
      }
    }
  }

  return TIndex();
}

vector<TIndex> Storage::FindAllIndexesByFile(string const & name) const
{
  EqualFileName fn(name);
  vector<TIndex> res;

  for (size_t i = 0; i < m_countries.SiblingsCount(); ++i)
  {
    if (fn(m_countries[i]))
      res.emplace_back(static_cast<int>(i));

    for (size_t j = 0; j < m_countries[i].SiblingsCount(); ++j)
    {
      if (fn(m_countries[i][j]))
        res.emplace_back(static_cast<int>(i), static_cast<int>(j));

      for (size_t k = 0; k < m_countries[i][j].SiblingsCount(); ++k)
      {
        if (fn(m_countries[i][j][k]))
          res.emplace_back(static_cast<int>(i), static_cast<int>(j), static_cast<int>(k));
      }
    }
  }

  return res;
}

void Storage::GetOutdatedCountries(vector<Country const *> & countries) const
{
  for (auto const & p : m_localFiles)
  {
    TIndex const & index = p.first;
    string const name = GetCountryFile(index).GetNameWithoutExt();
    TLocalFilePtr file = GetLatestLocalFile(index);
    if (file && file->GetVersion() != GetCurrentDataVersion() &&
        name != WORLD_COASTS_FILE_NAME && name != WORLD_FILE_NAME)
    {
      countries.push_back(&CountryByIndex(index));
    }
  }
}

TStatus Storage::CountryStatusWithoutFailed(TIndex const & index) const
{
  // First, check if we already downloading this country or have in in the queue.
  if (!IsCountryInQueue(index))
    return CountryStatusFull(index, TStatus::EUnknown);
  return IsCountryFirstInQueue(index) ? TStatus::EDownloading : TStatus::EInQueue;
}

TStatus Storage::CountryStatusFull(TIndex const & index, TStatus const status) const
{
  if (status != TStatus::EUnknown)
    return status;

  TLocalFilePtr localFile = GetLatestLocalFile(index);
  if (!localFile || !localFile->OnDisk(MapOptions::Map))
    return TStatus::ENotDownloaded;

  CountryFile const & countryFile = GetCountryFile(index);
  if (GetRemoteSize(countryFile, MapOptions::Map) == 0)
    return TStatus::EUnknown;

  if (localFile->GetVersion() != GetCurrentDataVersion())
    return TStatus::EOnDiskOutOfDate;
  return TStatus::EOnDisk;
}

MapOptions Storage::NormalizeDownloadFileSet(TIndex const & index, MapOptions options) const
{
  auto const & country = GetCountryFile(index);

  // Car routing files are useless without map files.
  if (HasOptions(options, MapOptions::CarRouting))
    options = SetOptions(options, MapOptions::Map);

  TLocalFilePtr localCountryFile = GetLatestLocalFile(index);
  for (MapOptions option : {MapOptions::Map, MapOptions::CarRouting})
  {
    // Check whether requested files are on disk and up-to-date.
    if (HasOptions(options, option) && localCountryFile && localCountryFile->OnDisk(option) &&
        localCountryFile->GetVersion() == GetCurrentDataVersion())
    {
      options = UnsetOptions(options, option);
    }

    // Check whether requested file is not empty.
    if (GetRemoteSize(country, option) == 0)
    {
      ASSERT_NOT_EQUAL(MapOptions::Map, option, ("Map can't be empty."));
      options = UnsetOptions(options, option);
    }
  }

  return options;
}

MapOptions Storage::NormalizeDeleteFileSet(MapOptions options) const
{
  // Car routing files are useless without map files.
  if (HasOptions(options, MapOptions::Map))
    options = SetOptions(options, MapOptions::CarRouting);
  return options;
}

QueuedCountry * Storage::FindCountryInQueue(TIndex const & index)
{
  auto it = find(m_queue.begin(), m_queue.end(), index);
  return it == m_queue.end() ? nullptr : &*it;
}

QueuedCountry const * Storage::FindCountryInQueue(TIndex const & index) const
{
  auto it = find(m_queue.begin(), m_queue.end(), index);
  return it == m_queue.end() ? nullptr : &*it;
}

bool Storage::IsCountryInQueue(TIndex const & index) const
{
  return FindCountryInQueue(index) != nullptr;
}

bool Storage::IsCountryFirstInQueue(TIndex const & index) const
{
  return !m_queue.empty() && m_queue.front().GetIndex() == index;
}

void Storage::SetDownloaderForTesting(unique_ptr<MapFilesDownloader> && downloader)
{
  m_downloader = move(downloader);
}

void Storage::SetCurrentDataVersionForTesting(int64_t currentVersion)
{
  m_currentVersion = currentVersion;
}

Storage::TLocalFilePtr Storage::GetLocalFile(TIndex const & index, int64_t version) const
{
  auto const it = m_localFiles.find(index);
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

  for (auto const & index : FindAllIndexesByFile(localFile->GetCountryName()))
  {
    TLocalFilePtr existingFile = GetLocalFile(index, localFile->GetVersion());
    if (existingFile)
      ASSERT_EQUAL(localFile.get(), existingFile.get(), ());
    else
      m_localFiles[index].push_front(localFile);
  }
}

void Storage::RegisterCountryFiles(TIndex const & index, string const & directory, int64_t version)
{
  TLocalFilePtr localFile = GetLocalFile(index, version);
  if (localFile)
    return;

  CountryFile const & countryFile = GetCountryFile(index);
  localFile = make_shared<LocalCountryFile>(directory, countryFile, version);
  RegisterCountryFiles(localFile);
}

void Storage::RegisterFakeCountryFiles(platform::LocalCountryFile const & localFile)
{
  TLocalFilePtr fakeCountryLocalFile = make_shared<LocalCountryFile>(localFile);
  fakeCountryLocalFile->SyncWithDisk();
  m_localFilesForFakeCountries[fakeCountryLocalFile->GetCountryFile()] = fakeCountryLocalFile;
}

void Storage::DeleteCountryFiles(TIndex const & index, MapOptions opt)
{
  auto const it = m_localFiles.find(index);
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
    m_localFiles.erase(index);
}

bool Storage::DeleteCountryFilesFromDownloader(TIndex const & index, MapOptions opt)
{
  QueuedCountry * queuedCountry = FindCountryInQueue(index);
  if (!queuedCountry)
    return false;

  opt = IntersectOptions(opt, queuedCountry->GetInitOptions());

  if (IsCountryFirstInQueue(index))
  {
    // Abrupt downloading of the current file if it should be removed.
    if (HasOptions(opt, queuedCountry->GetCurrentFile()))
      m_downloader->Reset();

    // Remove all files downloader had been created for a country.
    DeleteDownloaderFilesForCountry(GetCountryFile(index), GetCurrentDataVersion());
  }

  queuedCountry->RemoveOptions(opt);

  // Remove country from the queue if there's nothing to download.
  if (queuedCountry->GetInitOptions() == MapOptions::Nothing)
    m_queue.erase(find(m_queue.begin(), m_queue.end(), index));

  if (!m_queue.empty() && m_downloader->IsIdle())
  {
    // Kick possibly interrupted downloader.
    if (IsCountryFirstInQueue(index))
      DownloadNextFile(m_queue.front());
    else
      DownloadNextCountryFromQueue();
  }
  return true;
}

uint64_t Storage::GetDownloadSize(QueuedCountry const & queuedCountry) const
{
  CountryFile const & file = GetCountryFile(queuedCountry.GetIndex());
  return GetRemoteSize(file, queuedCountry.GetCurrentFile());
}

string Storage::GetFileDownloadPath(TIndex const & index, MapOptions file) const
{
  return platform::GetFileDownloadPath(GetCountryFile(index), file, GetCurrentDataVersion());
}
}  // namespace storage
