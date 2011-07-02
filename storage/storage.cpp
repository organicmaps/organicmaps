#include "storage.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"

#include "../indexer/data_header.hpp"

#include "../coding/file_writer.hpp"
#include "../coding/file_reader.hpp"
#include "../coding/file_container.hpp"
#include "../coding/url_encode.hpp"

#include "../version/version.hpp"

#include "../std/set.hpp"
#include "../std/algorithm.hpp"
#include "../std/target_os.hpp"
#include "../std/bind.hpp"

namespace storage
{
  const int TIndex::INVALID = -1;

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

  ////////////////////////////////////////////////////////////////////////////
  void Storage::Init(TAddMapFunction addFunc, TRemoveMapFunction removeFunc, TUpdateRectFunction updateRectFunc, TEnumMapsFunction enumMapsFunc)
  {
    m_currentVersion = static_cast<uint32_t>(Version::BUILD);

    m_addMap = addFunc;
    m_removeMap = removeFunc;
    m_updateRect = updateRectFunc;

    typedef vector<ModelReaderPtr> map_list_t;
    map_list_t filesList;
    enumMapsFunc(filesList);

    for (map_list_t::iterator it = filesList.begin(); it != filesList.end(); ++it)
      m_addMap(*it);
  }

  string Storage::UpdateBaseUrl() const
  {
    return UPDATE_BASE_URL OMIM_OS_NAME "/" + strings::to_string(m_currentVersion) + "/";
  }

  TCountriesContainer const & NodeFromIndex(TCountriesContainer const & root, TIndex const & index)
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

  size_t Storage::CountriesCount(TIndex const & index) const
  {
    return NodeFromIndex(m_countries, index).SiblingsCount();
  }

  string Storage::CountryName(TIndex const & index) const
  {
    return NodeFromIndex(m_countries, index).Value().Name();
  }

  TLocalAndRemoteSize Storage::CountrySizeInBytes(TIndex const & index) const
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
    if (m_failedCountries.find(index) != m_failedCountries.end())
      return EDownloadFailed;

    TLocalAndRemoteSize size = CountryByIndex(index).Size();
    if (size.first == size.second)
    {
      if (size.second == 0)
        return EUnknown;
      else
        return EOnDisk;
    }

    return ENotDownloaded;
  }

  void Storage::DownloadCountry(TIndex const & index)
  {
    // check if we already downloading this country
    TQueue::const_iterator found = find(m_queue.begin(), m_queue.end(), index);
    if (found != m_queue.end())
    { // do nothing
      return;
    }
    // remove it from failed list
    m_failedCountries.erase(index);
    // add it into the queue
    m_queue.push_back(index);
    // and start download if necessary
    if (m_queue.size() == 1)
    {
      // reset total country's download progress
      TLocalAndRemoteSize const size = CountryByIndex(index).Size();
      m_countryProgress.m_current = 0;
      m_countryProgress.m_total = size.second;

      DownloadNextCountryFromQueue();
    }
    else
    { // notify about "In Queue" status
      if (m_observerChange)
        m_observerChange(index);
    }
  }

  template <class TRemoveFn>
  class DeactivateMap
  {
    string m_workingDir;
    TRemoveFn & m_removeFn;
  public:
    DeactivateMap(TRemoveFn & removeFn) : m_removeFn(removeFn)
    {
      m_workingDir = GetPlatform().WritableDir();
    }
    void operator()(TTile const & tile)
    {
      string const file = m_workingDir + tile.first;
      m_removeFn(file);
    }
  };

  void Storage::DownloadNextCountryFromQueue()
  {
    while (!m_queue.empty())
    {
      TIndex index = m_queue.front();
      TTilesContainer const & tiles = CountryByIndex(index).Tiles();
      for (TTilesContainer::const_iterator it = tiles.begin(); it != tiles.end(); ++it)
      {
        if (!IsTileDownloaded(*it))
        {
          HttpStartParams params;
          params.m_url = UpdateBaseUrl() + UrlEncode(it->first);
          params.m_fileToSave = GetPlatform().WritablePathForFile(it->first);
          params.m_finish = bind(&Storage::OnMapDownloadFinished, this, _1);
          params.m_progress = bind(&Storage::OnMapDownloadProgress, this, _1);
          params.m_useResume = true;   // enabled resume support by default
          GetDownloadManager().HttpRequest(params);
          // notify GUI - new status for country, "Downloading"
          if (m_observerChange)
            m_observerChange(index);
          return;
        }
      }
      // continue with next country
      m_queue.pop_front();
      // reset total country's download progress
      if (!m_queue.empty())
      {
        m_countryProgress.m_current = 0;
        m_countryProgress.m_total = CountryByIndex(m_queue.front()).Size().second;
      }
      // and notify GUI - new status for country, "OnDisk"
      if (m_observerChange)
        m_observerChange(index);
    }
  }

  struct CancelDownloading
  {
    string const m_baseUrl;
    CancelDownloading(string const & baseUrl) : m_baseUrl(baseUrl) {}
    void operator()(TTile const & tile)
    {
      GetDownloadManager().CancelDownload((m_baseUrl + UrlEncode(tile.first)).c_str());
    }
  };

  class DeleteMap
  {
    string m_workingDir;
  public:
    DeleteMap()
    {
      m_workingDir = GetPlatform().WritableDir();
    }
    /// @TODO do not delete other countries cells
    void operator()(TTile const & tile)
    {
      FileWriter::DeleteFileX(m_workingDir + tile.first);
    }
  };

  template <typename TRemoveFunc>
  void DeactivateAndDeleteCountry(Country const & country, TRemoveFunc removeFunc)
  {
    // deactivate from multiindex
    for_each(country.Tiles().begin(), country.Tiles().end(), DeactivateMap<TRemoveFunc>(removeFunc));
    // delete from disk
    for_each(country.Tiles().begin(), country.Tiles().end(), DeleteMap());
  }

  m2::RectD Storage::CountryBounds(TIndex const & index) const
  {
    Country const & country = CountryByIndex(index);
    return country.Bounds();
  }

  void Storage::DeleteCountry(TIndex const & index)
  {
    Country const & country = CountryByIndex(index);

    m2::RectD bounds;

    // check if we already downloading this country
    TQueue::iterator found = find(m_queue.begin(), m_queue.end(), index);
    if (found != m_queue.end())
    {
      if (found == m_queue.begin())
      { // stop download
        for_each(country.Tiles().begin(), country.Tiles().end(), CancelDownloading(UpdateBaseUrl()));
        // remove from the queue
        m_queue.erase(found);
        // start another download if the queue is not empty
        DownloadNextCountryFromQueue();
      }
      else
      { // remove from the queue
        m_queue.erase(found);
      }
    }
    else
    {
      // bounds are only updated if country was already activated before
      bounds = country.Bounds();
    }

    DeactivateAndDeleteCountry(country, m_removeMap);
    if (m_observerChange)
      m_observerChange(index);

    if (bounds != m2::RectD::GetEmptyRect())
      m_updateRect(bounds);
  }

  void Storage::ReInitCountries(bool forceReload)
  {
    if (forceReload)
      m_countries.Clear();

    if (m_countries.SiblingsCount() == 0)
    {
      Platform & pl = GetPlatform();
      TTilesContainer tiles;
      if (LoadTiles(pl.GetReader(DATA_UPDATE_FILE), tiles, m_currentVersion))
      {
        if (!LoadCountries(pl.GetReader(COUNTRIES_FILE), tiles, m_countries))
        {
          LOG(LWARNING, ("Can't load countries file", COUNTRIES_FILE));
        }
      }
      else
      {
        LOG(LWARNING, ("Can't load update file", DATA_UPDATE_FILE));
      }
    }
  }

  void Storage::Subscribe(TObserverChangeCountryFunction change, TObserverProgressFunction progress,
                          TUpdateRequestFunction updateRequest)
  {
    m_observerChange = change;
    m_observerProgress = progress;
    m_observerUpdateRequest = updateRequest;

    ReInitCountries(false);
  }

  void Storage::Unsubscribe()
  {
    m_observerChange.clear();
    m_observerProgress.clear();
    m_observerUpdateRequest.clear();
  }

  void Storage::OnMapDownloadFinished(HttpFinishedParams const & result)
  {
    if (m_queue.empty())
    {
      ASSERT(false, ("Invalid url?", result.m_url));
      return;
    }

    if (result.m_error != EHttpDownloadOk)
    {
      // remove failed country from the queue
      TIndex failedIndex = m_queue.front();
      m_queue.pop_front();
      m_failedCountries.insert(failedIndex);
      // notify GUI about failed country
      if (m_observerChange)
        m_observerChange(failedIndex);
    }
    else
    {
      TLocalAndRemoteSize size = CountryByIndex(m_queue.front()).Size();
      if (size.second != 0)
        m_countryProgress.m_current = size.first;

      /// @todo Get file reader from download framework.
      // activate downloaded map piece
      m_addMap(new FileReader(result.m_file));

      feature::DataHeader header;
      header.Load(FilesContainerR(result.m_file).GetReader(HEADER_FILE_TAG));
      m_updateRect(header.GetBounds());
    }
    DownloadNextCountryFromQueue();
  }

  void Storage::OnMapDownloadProgress(HttpProgressT const & progress)
  {
    if (m_queue.empty())
    {
      ASSERT(false, ("queue can't be empty"));
      return;
    }

    if (m_observerProgress)
    {
      HttpProgressT p(progress);
      p.m_current = m_countryProgress.m_current + progress.m_current;
      p.m_total = m_countryProgress.m_total;
      m_observerProgress(m_queue.front(), p);
    }
  }

  void Storage::CheckForUpdate()
  {
    // at this moment we support only binary update checks
    string const update = UpdateBaseUrl() + BINARY_UPDATE_FILE/*DATA_UPDATE_FILE*/;
    GetDownloadManager().CancelDownload(update);
    HttpStartParams params;
    params.m_url = update;
    params.m_fileToSave = GetPlatform().WritablePathForFile(DATA_UPDATE_FILE);
    params.m_finish = bind(&Storage::OnBinaryUpdateCheckFinished, this, _1);
    params.m_useResume = false;
    GetDownloadManager().HttpRequest(params);
  }

  void Storage::OnDataUpdateCheckFinished(HttpFinishedParams const & params)
  {
    if (params.m_error != EHttpDownloadOk)
    {
      LOG(LWARNING, ("Update check failed for url:", params.m_url));
      if (m_observerUpdateRequest)
        m_observerUpdateRequest(EDataCheckFailed, ErrorString(params.m_error));
    }
    else
    { // @TODO parse update file and notify GUI
    }

    // parse update file
//    TCountriesContainer tempCountries;
//    if (!LoadCountries(tempCountries, GetPlatform().WritablePathForFile(DATA_UPDATE_FILE)))
//    {
//      LOG(LWARNING, ("New application version should be downloaded, "
//                     "update file format can't be parsed"));
//      // @TODO: report to GUI
//      return;
//    }
//    // stop any active download, clear the queue, replace countries and notify GUI
//    if (!m_queue.empty())
//    {
//      CancelCountryDownload(CountryByIndex(m_queue.front()));
//      m_queue.clear();
//    }
//    m_countries.swap(tempCountries);
//    // @TODO report to GUI about reloading all countries
//    LOG(LINFO, ("Update check complete"));
  }

  void Storage::OnBinaryUpdateCheckFinished(HttpFinishedParams const & params)
  {
    if (params.m_error == EHttpDownloadFileNotFound)
    {
      // no binary update is available
      if (m_observerUpdateRequest)
        m_observerUpdateRequest(ENoAnyUpdateAvailable, "No update is available");
    }
    else if (params.m_error == EHttpDownloadOk)
    {
      // update is available!
      try
      {
        if (m_observerUpdateRequest)
        {
          string buffer;
          ReaderPtr<Reader>(GetPlatform().GetReader(params.m_file)).ReadAsString(buffer);
          m_observerUpdateRequest(ENewBinaryAvailable, buffer);
        }
      }
      catch (std::exception const & e)
      {
        if (m_observerUpdateRequest)
          m_observerUpdateRequest(EBinaryCheckFailed,
                                    string("Error loading b-update text file ") + e.what());
      }
    }
    else
    {
      // connection error
      if (m_observerUpdateRequest)
        m_observerUpdateRequest(EBinaryCheckFailed, ErrorString(params.m_error));
    }
  }
}
