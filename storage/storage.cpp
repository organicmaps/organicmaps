#include "storage/storage.hpp"

#include "storage/http_map_files_downloader.hpp"

#include "defines.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"
#include "platform/servers_list.hpp"
#include "platform/settings.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/url_encode.hpp"

#include "platform/local_country_file_utils.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/target_os.hpp"
#include "std/bind.hpp"
#include "std/sstream.hpp"

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

uint64_t GetLocalSize(shared_ptr<LocalCountryFile> file, TMapOptions opt)
{
  if (file.get() == nullptr)
    return 0;
  uint64_t size = 0;
  for (TMapOptions bit : {TMapOptions::EMap, TMapOptions::ECarRouting})
  {
    if (HasOptions(opt, bit))
      size += file->GetSize(bit);
  }
  return size;
}

uint64_t GetRemoteSize(CountryFile const & file, TMapOptions opt)
{
  uint64_t size = 0;
  for (TMapOptions bit : {TMapOptions::EMap, TMapOptions::ECarRouting})
  {
    if (HasOptions(opt, bit))
      size += file.GetRemoteSize(bit);
  }
  return size;
}

// TODO (@gorshenin): directory where country indexes are stored
// should be abstracted out to LocalCountryIndexes.
void DeleteCountryIndexes(CountryFile const & file)
{
  Platform::FilesList files;
  Platform const & platform = GetPlatform();
  string const name = file.GetNameWithoutExt();
  string const path = platform.WritablePathForCountryIndexes(name);

  /// @todo We need correct regexp for any file (not including "." and "..").
  platform.GetFilesByRegExp(path, name + "\\..*", files);
  for (auto const & file : files)
    my::DeleteFileX(path + file);
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

void Storage::Init(TUpdateAfterDownload const & updateFn) { m_updateAfterDownload = updateFn; }

void Storage::RegisterAllLocalMaps()
{
  m_localFiles.clear();
  m_localFilesForFakeCountries.clear();

  vector<LocalCountryFile> localFiles;
  FindAllLocalMaps(localFiles);

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
      localFile.DeleteFromDisk(TMapOptions::EMapWithCarRouting);
      ++j;
    }

    LocalCountryFile const & localFile = *i;
    string const name = localFile.GetCountryFile().GetNameWithoutExt();
    TIndex index = FindIndexByFile(name);
    if (index.IsValid())
      RegisterCountryFiles(index, localFile.GetDirectory(), localFile.GetVersion());
    else
      RegisterFakeCountryFiles(localFile);
    LOG(LINFO, ("Found file:", name, "in directory:", localFile.GetDirectory()));

    i = j;
  }
}

void Storage::GetLocalMaps(vector<CountryFile> & maps)
{
  for (auto const & p : m_localFiles)
  {
    TIndex const & index = p.first;
    maps.push_back(GetLatestLocalFile(index)->GetCountryFile());
  }
  for (auto const & p : m_localFilesForFakeCountries)
    maps.push_back(p.second->GetCountryFile());
}

CountriesContainerT const & NodeFromIndex(CountriesContainerT const & root, TIndex const & index)
{
  // complex logic to avoid [] out_of_bounds exceptions
  if (index.m_group == TIndex::INVALID || index.m_group >= static_cast<int>(root.SiblingsCount()))
    return root;
  else
  {
    if (index.m_country == TIndex::INVALID ||
        index.m_country >= static_cast<int>(root[index.m_group].SiblingsCount()))
      return root[index.m_group];
    if (index.m_region == TIndex::INVALID ||
        index.m_region >= static_cast<int>(root[index.m_group][index.m_country].SiblingsCount()))
      return root[index.m_group][index.m_country];
    return root[index.m_group][index.m_country][index.m_region];
  }
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

LocalAndRemoteSizeT Storage::CountrySizeInBytes(TIndex const & index, TMapOptions opt) const
{
  QueuedCountry const * queuedCountry = FindCountryInQueue(index);
  shared_ptr<LocalCountryFile> localFile = GetLatestLocalFile(index);
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

shared_ptr<LocalCountryFile> Storage::GetLatestLocalFile(CountryFile const & countryFile) const
{
  TIndex const index = FindIndexByFile(countryFile.GetNameWithoutExt());
  {
    shared_ptr<LocalCountryFile> localFile = GetLatestLocalFile(index);
    if (localFile.get())
      return localFile;
  }
  {
    auto const it = m_localFilesForFakeCountries.find(countryFile);
    if (it != m_localFilesForFakeCountries.end())
      return it->second;
  }
  return shared_ptr<LocalCountryFile>();
}

shared_ptr<LocalCountryFile> Storage::GetLatestLocalFile(TIndex const & index) const
{
  auto const it = m_localFiles.find(index);
  if (it == m_localFiles.end() || it->second.empty())
    return shared_ptr<LocalCountryFile>();
  list<shared_ptr<LocalCountryFile>> const & files = it->second;
  shared_ptr<LocalCountryFile> latest = files.front();
  for (shared_ptr<LocalCountryFile> const & file : files)
  {
    if (file->GetVersion() > latest->GetVersion())
      latest = file;
  }
  return latest;
}

TStatus Storage::CountryStatus(TIndex const & index) const
{
  // first, check if we already downloading this country or have in in the queue
  if (IsCountryInQueue(index))
  {
    if (IsCountryFirstInQueue(index))
      return TStatus::EDownloading;
    else
      return TStatus::EInQueue;
  }

  // second, check if this country has failed while downloading
  if (m_failedCountries.count(index) > 0)
    return TStatus::EDownloadFailed;

  return TStatus::EUnknown;
}

TStatus Storage::CountryStatusEx(TIndex const & index) const
{
  return CountryStatusFull(index, CountryStatus(index));
}

void Storage::CountryStatusEx(TIndex const & index, TStatus & status, TMapOptions & options) const
{
  status = CountryStatusEx(index);

  if (status == TStatus::EOnDisk || status == TStatus::EOnDiskOutOfDate)
  {
    options = TMapOptions::EMap;

    shared_ptr<LocalCountryFile> localFile = GetLatestLocalFile(index);
    ASSERT(localFile.get(), ("Invariant violation: local file out of sync with disk."));
    if (localFile->OnDisk(TMapOptions::ECarRouting))
      options = SetOptions(options, TMapOptions::ECarRouting);
  }
}

void Storage::DownloadCountry(TIndex const & index, TMapOptions opt)
{
  opt = NormalizeDownloadFileSet(index, opt);
  if (opt == TMapOptions::ENothing)
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

void Storage::DeleteCountry(TIndex const & index, TMapOptions opt)
{
  opt = NormalizeDeleteFileSet(opt);
  DeleteCountryFiles(index, opt);
  DeleteCountryFilesFromDownloader(index, opt);
  KickDownloaderAfterDeletionOfCountryFiles(index);
  NotifyStatusChanged(index);
}

void Storage::DeleteCustomCountryVersion(LocalCountryFile const & localFile)
{
  CountryFile const countryFile = localFile.GetCountryFile();
  localFile.DeleteFromDisk(TMapOptions::EMapWithCarRouting);

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
    DeleteCountryFilesFromDownloader(index, TMapOptions::EMapWithCarRouting);
  auto countryFilesIt = m_localFiles.find(index);
  if (countryFilesIt == m_localFiles.end())
  {
    LOG(LERROR, ("Deleted files of an unregistered country:", localFile));
    return;
  }

  auto equalsToLocalFile = [&localFile](shared_ptr<LocalCountryFile> const & rhs)
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
  DownloadNextFile(queuedCountry);

  // New status for the country, "Downloading"
  NotifyStatusChanged(queuedCountry.GetIndex());
}

void Storage::DownloadNextFile(QueuedCountry const & country)
{
  CountryFile const & countryFile = GetCountryFile(country.GetIndex());

  // send Country name for statistics
  m_downloader->GetServersList(countryFile.GetNameWithoutExt(),
                               bind(&Storage::OnServerListDownloaded, this, _1));
}

bool Storage::DeleteFromDownloader(TIndex const & index)
{
  if (!DeleteCountryFilesFromDownloader(index, TMapOptions::EMapWithCarRouting))
    return false;
  KickDownloaderAfterDeletionOfCountryFiles(index);
  NotifyStatusChanged(index);
  return true;
}

bool Storage::IsDownloadInProgress() const { return !m_queue.empty(); }

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

  Platform & platform = GetPlatform();
  string const path = GetFileDownloadPath(index, queuedCountry.GetCurrentFile());

  success = success && platform.IsFileExistsByFullPath(path) &&
            RegisterDownloadedFile(path, progress.first /* size */, GetCurrentDataVersion());

  if (success && queuedCountry.SwitchToNextFile())
  {
    DownloadNextFile(queuedCountry);
    return;
  }

  {
    string optionsName;
    switch (queuedCountry.GetInitOptions())
    {
      case TMapOptions::ENothing:
        optionsName = "Nothing";
        break;
      case TMapOptions::EMap:
        optionsName = "Map";
        break;
      case TMapOptions::ECarRouting:
        optionsName = "CarRouting";
        break;
      case TMapOptions::EMapWithCarRouting:
        optionsName = "MapWithCarRouting";
        break;
    }
    alohalytics::LogEvent(
        "$OnMapDownloadFinished",
        alohalytics::TStringMap({{"name", GetCountryFile(index).GetNameWithoutExt()},
                                 {"status", success ? "ok" : "failed"},
                                 {"version", strings::to_string(GetCurrentDataVersion())},
                                 {"option", optionsName}}));
  }
  if (success)
  {
    shared_ptr<LocalCountryFile> localFile = GetLocalFile(index, GetCurrentDataVersion());
    ASSERT(localFile.get(), ());
    OnMapDownloadFinished(localFile);
  }
  else
  {
    OnMapDownloadFailed();
  }

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
  TMapOptions const file = queuedCountry.GetCurrentFile();

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

bool Storage::RegisterDownloadedFile(string const & path, uint64_t size, int64_t version)
{
  QueuedCountry & queuedCountry = m_queue.front();
  TIndex const & index = queuedCountry.GetIndex();
  uint64_t const expectedSize = GetDownloadSize(queuedCountry);

  ASSERT_EQUAL(size, expectedSize, ("Downloaded file size mismatch:", size,
                                    "bytes were downloaded,", expectedSize, "bytes expected."));

  CountryFile const countryFile = GetCountryFile(index);
  shared_ptr<LocalCountryFile> localFile = GetLocalFile(index, version);
  if (localFile.get() == nullptr)
    localFile = PreparePlaceForCountryFiles(countryFile, version);
  if (localFile.get() == nullptr)
  {
    LOG(LERROR, ("Local file data structure can't prepared for downloaded file(", path, ")."));
    return false;
  }
  if (!my::RenameFileX(path, localFile->GetPath(queuedCountry.GetCurrentFile())))
    return false;
  RegisterCountryFiles(localFile);
  return true;
}

void Storage::OnMapDownloadFinished(shared_ptr<LocalCountryFile> localFile)
{
  DeleteCountryIndexes(localFile->GetCountryFile());

  // Notify framework that all requested files for the country were downloaded.
  m_updateAfterDownload(*localFile);
}

void Storage::OnMapDownloadFailed()
{
  TIndex const & index = m_queue.front().GetIndex();

  // Add country to the failed countries set.
  m_failedCountries.insert(index);
  m_downloader->Reset();
  DownloadNextCountryFromQueue();
}

string Storage::GetFileDownloadUrl(string const & baseUrl, TIndex const & index,
                                   TMapOptions file) const
{
  CountryFile const & countryFile = GetCountryFile(index);
  return baseUrl + OMIM_OS_NAME "/" + strings::to_string(GetCurrentDataVersion()) + "/" +
         UrlEncode(countryFile.GetNameWithExt(file));
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
    shared_ptr<LocalCountryFile> const & file = GetLatestLocalFile(index);
    if (file.get() && file->GetVersion() != GetCurrentDataVersion() &&
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

  shared_ptr<LocalCountryFile> localFile = GetLatestLocalFile(index);
  if (localFile.get() == nullptr || !localFile->OnDisk(TMapOptions::EMap))
    return TStatus::ENotDownloaded;

  CountryFile const & countryFile = GetCountryFile(index);
  if (GetRemoteSize(countryFile, TMapOptions::EMap) == 0)
    return TStatus::EUnknown;

  if (localFile->GetVersion() != GetCurrentDataVersion())
    return TStatus::EOnDiskOutOfDate;
  return TStatus::EOnDisk;
}

TMapOptions Storage::NormalizeDownloadFileSet(TIndex const & index, TMapOptions opt) const
{
  // Car routing files are useless without map files.
  if (HasOptions(opt, TMapOptions::ECarRouting))
    opt = SetOptions(opt, TMapOptions::EMap);

  shared_ptr<LocalCountryFile> localCountryFile = GetLatestLocalFile(index);
  for (TMapOptions file : {TMapOptions::EMap, TMapOptions::ECarRouting})
  {
    // Check whether requested files are on disk and up-to-date.
    if (HasOptions(opt, file) && localCountryFile.get() && localCountryFile->OnDisk(file) &&
        localCountryFile->GetVersion() == GetCurrentDataVersion())
    {
      opt = UnsetOptions(opt, file);
    }
  }
  return opt;
}

TMapOptions Storage::NormalizeDeleteFileSet(TMapOptions opt) const
{
  // Car routing files are useless without map files.
  if (HasOptions(opt, TMapOptions::EMap))
    opt = SetOptions(opt, TMapOptions::ECarRouting);
  return opt;
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

shared_ptr<LocalCountryFile> Storage::GetLocalFile(TIndex const & index, int64_t version) const
{
  auto const it = m_localFiles.find(index);
  if (it == m_localFiles.end() || it->second.empty())
    return shared_ptr<LocalCountryFile>();
  list<shared_ptr<LocalCountryFile>> const & files = it->second;
  for (shared_ptr<LocalCountryFile> const & file : files)
  {
    if (file->GetVersion() == version)
      return file;
  }
  return shared_ptr<LocalCountryFile>();
}

void Storage::RegisterCountryFiles(shared_ptr<LocalCountryFile> localFile)
{
  ASSERT(localFile.get(), ());
  localFile->SyncWithDisk();

  TIndex const index = FindIndexByFile(localFile->GetCountryFile().GetNameWithoutExt());
  shared_ptr<LocalCountryFile> existingFile = GetLocalFile(index, localFile->GetVersion());
  if (existingFile.get() != nullptr)
    ASSERT_EQUAL(localFile.get(), existingFile.get(), ());
  else
    m_localFiles[index].push_front(localFile);
}

void Storage::RegisterCountryFiles(TIndex const & index, string const & directory, int64_t version)
{
  shared_ptr<LocalCountryFile> localFile = GetLocalFile(index, version);
  if (localFile.get() != nullptr)
    return;

  CountryFile const & countryFile = GetCountryFile(index);
  localFile = make_shared<LocalCountryFile>(directory, countryFile, version);
  RegisterCountryFiles(localFile);
}

void Storage::RegisterFakeCountryFiles(platform::LocalCountryFile const & localFile)
{
  shared_ptr<LocalCountryFile> fakeCountryLocalFile = make_shared<LocalCountryFile>(localFile);
  fakeCountryLocalFile->SyncWithDisk();
  m_localFilesForFakeCountries[fakeCountryLocalFile->GetCountryFile()] = fakeCountryLocalFile;
}

void Storage::DeleteCountryFiles(TIndex const & index, TMapOptions opt)
{
  auto const it = m_localFiles.find(index);
  if (it == m_localFiles.end())
    return;

  // TODO (@gorshenin): map-only indexes should not be touched when
  // routing indexes are removed.
  if (!it->second.empty())
    DeleteCountryIndexes(it->second.front()->GetCountryFile());

  list<shared_ptr<platform::LocalCountryFile>> & localFiles = it->second;
  for (shared_ptr<LocalCountryFile> & localFile : localFiles)
  {
    localFile->DeleteFromDisk(opt);
    localFile->SyncWithDisk();
    if (localFile->GetFiles() == TMapOptions::ENothing)
      localFile.reset();
  }
  auto isNull = [](shared_ptr<LocalCountryFile> const & localFile)
  {
    return !localFile.get();
  };
  localFiles.remove_if(isNull);
  if (localFiles.empty())
    m_localFiles.erase(index);
}

bool Storage::DeleteCountryFilesFromDownloader(TIndex const & index, TMapOptions opt)
{
  QueuedCountry * queuedCountry = FindCountryInQueue(index);
  if (!queuedCountry)
    return false;
  if (IsCountryFirstInQueue(index))
  {
    // Abrupt downloading of the current file if it should be removed.
    if (HasOptions(opt, queuedCountry->GetCurrentFile()))
      m_downloader->Reset();
  }
  queuedCountry->RemoveOptions(opt);

  // Remove country from the queue if there's nothing to download.
  if (queuedCountry->GetInitOptions() == TMapOptions::ENothing)
    m_queue.erase(find(m_queue.begin(), m_queue.end(), index));
  return true;
}

void Storage::KickDownloaderAfterDeletionOfCountryFiles(TIndex const & index)
{
  // Do nothing when there're no counties to download or when downloader is busy.
  if (m_queue.empty() || !m_downloader->IsIdle())
    return;
  if (IsCountryFirstInQueue(index))
    DownloadNextFile(m_queue.front());
  else
    DownloadNextCountryFromQueue();
}

uint64_t Storage::GetDownloadSize(QueuedCountry const & queuedCountry) const
{
  CountryFile const & file = GetCountryFile(queuedCountry.GetIndex());
  return GetRemoteSize(file, queuedCountry.GetCurrentFile());
}

string Storage::GetFileDownloadPath(TIndex const & index, TMapOptions file) const
{
  Platform & platform = GetPlatform();
  CountryFile const & countryFile = GetCountryFile(index);
  return platform.WritablePathForFile(countryFile.GetNameWithExt(file) + READY_FILE_EXTENSION);
}
}  // namespace storage
