#include "storage.hpp"

#include "../defines.hpp"

#include "../platform/platform.hpp"
#include "../platform/servers_list.hpp"
#include "../platform/settings.hpp"

#include "../coding/file_writer.hpp"
#include "../coding/file_reader.hpp"
#include "../coding/file_container.hpp"
#include "../coding/url_encode.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"

#include "../std/algorithm.hpp"
#include "../std/target_os.hpp"
#include "../std/bind.hpp"
#include "../std/sstream.hpp"


using namespace downloader;

namespace storage
{
  /*
  static string ErrorString(DownloadResultT res)
  {
    switch (res)
    {
    case EHttpDownloadCantCreateFile:
      return "File can't be created. Probably, you have no disk space available or "
                         "using read-only file system.";
    case EHttpDownloadFailed:
      return "Download failed due to missing or poor connection. "
                         "Please, try again later.";
    case EHttpDownloadFileIsLocked:
      return "Download can't be finished because file is locked. "
                         "Please, try again after restarting application.";
    case EHttpDownloadFileNotFound:
      return "Requested file is absent on the server.";
    case EHttpDownloadNoConnectionAvailable:
      return "No network connection is available.";
    case EHttpDownloadOk:
      return "Download finished successfully.";
    }
    return "Unknown error";
  }
  */

  Storage::Storage() : m_currentSlotId(0)
  {
    LoadCountriesFile(false);

    if (Settings::IsFirstLaunchForDate(121031))
    {
      Platform & pl = GetPlatform();
      string const dir = pl.WritableDir();

      // Delete all: .mwm.downloading; .mwm.downloading2; .mwm.resume; .mwm.resume2
      string const regexp = "\\" DATA_FILE_EXTENSION "\\.(downloading2?$|resume2?$)";

      Platform::FilesList files;
      pl.GetFilesByRegExp(dir, regexp, files);

      for (size_t j = 0; j < files.size(); ++j)
        FileWriter::DeleteFileX(dir + files[j]);
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  void Storage::Init(TUpdateAfterDownload const & updateFn)
  {
    m_updateAfterDownload = updateFn;
  }

  CountriesContainerT const & NodeFromIndex(CountriesContainerT const & root, TIndex const & index)
  {
    // complex logic to avoid [] out_of_bounds exceptions
    if (index.m_group == TIndex::INVALID || index.m_group >= static_cast<int>(root.SiblingsCount()))
      return root;
    else
    {
      if (index.m_country == TIndex::INVALID || index.m_country >= static_cast<int>(root[index.m_group].SiblingsCount()))
        return root[index.m_group];
      if (index.m_region == TIndex::INVALID || index.m_region >= static_cast<int>(root[index.m_group][index.m_country].SiblingsCount()))
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
    string fName = CountryByIndex(index).GetFile().m_fileName;
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

  LocalAndRemoteSizeT Storage::CountrySizeInBytes(TIndex const & index) const
  {
    return CountryByIndex(index).Size();
  }

  TStatus Storage::CountryStatus(TIndex const & index) const
  {
    // first, check if we already downloading this country or have in in the queue
    TQueue::const_iterator found = std::find(m_queue.begin(), m_queue.end(), index);
    if (found != m_queue.end())
    {
      if (found == m_queue.begin())
        return EDownloading;
      else
        return EInQueue;
    }

    // second, check if this country has failed while downloading
    if (m_failedCountries.count(index) > 0)
      return EDownloadFailed;

    return EUnknown;
  }

  void Storage::DownloadCountry(TIndex const & index)
  {
    // check if we already downloading this country
    TQueue::const_iterator found = find(m_queue.begin(), m_queue.end(), index);
    if (found != m_queue.end())
    {
      // do nothing
      return;
    }

    // remove it from failed list
    m_failedCountries.erase(index);
    // add it into the queue
    m_queue.push_back(index);

    // and start download if necessary
    if (m_queue.size() == 1)
    {
      DownloadNextCountryFromQueue();
    }
    else
    {
      // notify about "In Queue" status
      NotifyStatusChanged(index);
    }
  }

  void Storage::NotifyStatusChanged(TIndex const & index) const
  {
    for (list<CountryObservers>::const_iterator it = m_observers.begin(); it != m_observers.end(); ++it)
      it->m_changeCountryFn(index);
  }

  void Storage::DownloadNextCountryFromQueue()
  {
    if (!m_queue.empty())
    {
      TIndex const index = m_queue.front();
      Country const & country = CountryByIndex(index);

      /// Reset progress before downloading.
      /// @todo If we will have more than one file per country,
      /// we should initialize progress before calling DownloadNextCountryFromQueue().
      m_countryProgress.first = 0;
      m_countryProgress.second = country.Size().second;

      // send Country name for statistics
      m_request.reset(HttpRequest::PostJson(GetPlatform().MetaServerUrl(),
              country.GetFile().m_fileName,
              bind(&Storage::OnServerListDownloaded, this, _1)));

      // new status for country, "Downloading"
      NotifyStatusChanged(index);
    }
  }

  /*
  m2::RectD Storage::CountryBounds(TIndex const & index) const
  {
    Country const & country = CountryByIndex(index);
    return country.Bounds();
  }
  */

  bool Storage::DeleteFromDownloader(TIndex const & index)
  {
    // check if we already downloading this country
    TQueue::iterator found = find(m_queue.begin(), m_queue.end(), index);
    if (found != m_queue.end())
    {
      if (found == m_queue.begin())
      {
        // stop download
        m_request.reset();
        // remove from the queue
        m_queue.erase(found);
        // start another download if the queue is not empty
        DownloadNextCountryFromQueue();
      }
      else
      {
        // remove from the queue
        m_queue.erase(found);
      }

      return true;
    }

    return false;
  }

  bool Storage::IsDownloadInProgress() const
  {
    return !m_queue.empty();
  }

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

  int Storage::Subscribe(TChangeCountryFunction const & change,
                         TProgressFunction const & progress)
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
    for (ObserversContT::iterator i = m_observers.begin(); i != m_observers.end(); ++i)
    {
      if (i->m_slotId == slotId)
      {
        m_observers.erase(i);
        return;
      }
    }
  }

  void Storage::OnMapDownloadFinished(HttpRequest & request)
  {
    if (m_queue.empty())
    {
      ASSERT ( false, ("queue can't be empty") );
      return;
    }

    TIndex const index = m_queue.front();
    m_queue.pop_front();

    if (request.Status() == HttpRequest::EFailed)
    {
      // add to failed countries set
      m_failedCountries.insert(index);
    }
    else
    {
      Country const & country = CountryByIndex(index);

      // notify framework that downloading is done
      m_updateAfterDownload(country.GetFile().GetFileWithExt());
    }

    NotifyStatusChanged(index);

    m_request.reset();
    DownloadNextCountryFromQueue();
  }

  void Storage::ReportProgress(TIndex const & idx, pair<int64_t, int64_t> const & p)
  {
    for (ObserversContT::const_iterator i = m_observers.begin(); i != m_observers.end(); ++i)
      i->m_progressFn(idx, p);
  }

  void Storage::OnMapDownloadProgress(HttpRequest & request)
  {
    if (m_queue.empty())
    {
      ASSERT ( false, ("queue can't be empty") );
      return;
    }

    if (!m_observers.empty())
    {
      HttpRequest::ProgressT p = request.Progress();
      p.first += m_countryProgress.first;
      p.second = m_countryProgress.second;

      ReportProgress(m_queue.front(), p);
    }
  }

  void Storage::OnServerListDownloaded(HttpRequest & request)
  {
    if (m_queue.empty())
    {
      ASSERT ( false, ("queue can't be empty") );
      return;
    }

    CountryFile const & file = CountryByIndex(m_queue.front()).GetFile();

    vector<string> urls;
    GetServerListFromRequest(request, urls);

    // append actual version and file name
    for (size_t i = 0; i < urls.size(); ++i)
      urls[i] = GetFileDownloadUrl(urls[i], file.GetFileWithExt());

    string const fileName = GetPlatform().WritablePathForFile(file.GetFileWithExt() + READY_FILE_EXTENSION);
    m_request.reset(HttpRequest::GetFile(urls, fileName, file.m_remoteSize,
                                         bind(&Storage::OnMapDownloadFinished, this, _1),
                                         bind(&Storage::OnMapDownloadProgress, this, _1)));
    ASSERT ( m_request, () );
  }

  string Storage::GetFileDownloadUrl(string const & baseUrl, string const & fName) const
  {
    return baseUrl + OMIM_OS_NAME "/" + strings::to_string(m_currentVersion)  + "/" + UrlEncode(fName);
  }

  bool IsEqualFileName(SimpleTree<Country> const & node, string const & name)
  {
    Country const & c = node.Value();
    if (c.GetFilesCount() > 0)
      return (c.GetFile().m_fileName == name);
    else
      return false;
  }

  TIndex Storage::FindIndexByFile(string const & name) const
  {
    for (unsigned i = 0; i < m_countries.SiblingsCount(); ++i)
    {
      if (IsEqualFileName(m_countries[i], name))
        return TIndex(i);

      for (unsigned j = 0; j < m_countries[i].SiblingsCount(); ++j)
      {
        if (IsEqualFileName(m_countries[i][j], name))
          return TIndex(i, j);

        for (unsigned k = 0; k < m_countries[i][j].SiblingsCount(); ++k)
        {
          if (IsEqualFileName(m_countries[i][j][k], name))
            return TIndex(i, j, k);
        }
      }
    }

    return TIndex();
  }

  static bool IsNotUpdatable(string const & t)
  {
    return (t == WORLD_COASTS_FILE_NAME) || (t == WORLD_FILE_NAME);
  }

  static string RemoveExt(string const & s)
  {
    return s.substr(0, s.find_last_of('.'));
  }

  class IsNotOutdatedFilter
  {
    Storage const & m_storage;

  public:
    IsNotOutdatedFilter(Storage const & storage)
      : m_storage(storage)
    {}

    bool operator()(string const & fileName)
    {
      const TIndex index = m_storage.FindIndexByFile(fileName);
      TStatus res = m_storage.CountryStatus(index);

      if (res == EUnknown)
      {
        Country const & c = m_storage.CountryByIndex(index);
        LocalAndRemoteSizeT const size = c.Size();

        if (size.first == 0)
          return ENotDownloaded;

        if (size.second == 0)
          return EUnknown;

        res = EOnDisk;
        if (size.first != size.second)
        {
          /// @todo Do better version check, not just size comparison.

          // Additional check for .ready file.
          // Use EOnDisk status if it's good, or EOnDiskOutOfDate otherwise.
          Platform const & pl = GetPlatform();
          string const fName = pl.WritablePathForFile(c.GetFile().GetFileWithExt() + READY_FILE_EXTENSION);

          uint64_t sz = 0;
          if (!pl.GetFileSizeByFullPath(fName, sz) || sz != size.second)
            res = EOnDiskOutOfDate;
        }
      }
      return res != EOnDiskOutOfDate;
    }
  };

  int Storage::GetOutdatedCountries(vector<Country> & list) const
  {
    Platform & pl = GetPlatform();
    Platform::FilesList fList;
    pl.GetFilesByExt(pl.WritableDir(), DATA_FILE_EXTENSION, fList);

    Platform::FilesList fListNoExt(fList.size());
    transform(fList.begin(), fList.end(), fListNoExt.begin(), RemoveExt);
    fListNoExt.erase(remove_if(fListNoExt.begin(), fListNoExt.end(), IsNotUpdatable), fListNoExt.end());
    fListNoExt.erase(remove_if(fListNoExt.begin(), fListNoExt.end(), IsNotOutdatedFilter(*this)), fListNoExt.end());

    for (int i = 0; i < fListNoExt.size(); ++i)
      list.push_back(CountryByIndex(FindIndexByFile(fListNoExt[i])));

    return fListNoExt.size();
  }

  int64_t Storage::GetCurrentDataVersion() const
  {
    return m_currentVersion;
  }
}
