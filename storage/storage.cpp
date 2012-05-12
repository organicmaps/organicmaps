#include "storage.hpp"

#include "../defines.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"

#include "../indexer/data_factory.hpp"
#include "../indexer/search_index_builder.hpp"

#include "../platform/platform.hpp"
#include "../platform/settings.hpp"

#include "../coding/file_writer.hpp"
#include "../coding/file_reader.hpp"
#include "../coding/file_container.hpp"
#include "../coding/url_encode.hpp"

#include "../version/version.hpp"

#include "../std/algorithm.hpp"
#include "../std/target_os.hpp"
#include "../std/bind.hpp"
#include "../std/sstream.hpp"

#include "../3party/jansson/myjansson.hpp"

#define SETTINGS_SERVERS_KEY "LastBaseUrls"

using namespace downloader;

namespace storage
{
  const int TIndex::INVALID = -1;

//  static string ErrorString(DownloadResultT res)
//  {
//    switch (res)
//    {
//    case EHttpDownloadCantCreateFile:
//      return "File can't be created. Probably, you have no disk space available or "
//                         "using read-only file system.";
//    case EHttpDownloadFailed:
//      return "Download failed due to missing or poor connection. "
//                         "Please, try again later.";
//    case EHttpDownloadFileIsLocked:
//      return "Download can't be finished because file is locked. "
//                         "Please, try again after restarting application.";
//    case EHttpDownloadFileNotFound:
//      return "Requested file is absent on the server.";
//    case EHttpDownloadNoConnectionAvailable:
//      return "No network connection is available.";
//    case EHttpDownloadOk:
//      return "Download finished successfully.";
//    }
//    return "Unknown error";
//  }

  Storage::Storage()
  {
    LoadCountriesFile(false);
  }

  ////////////////////////////////////////////////////////////////////////////
  void Storage::Init(TAddMapFunction addFunc, TRemoveMapFunction removeFunc, TUpdateRectFunction updateRectFunc)
  {
    m_addMap = addFunc;
    m_removeMap = removeFunc;
    m_updateRect = updateRectFunc;
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

    if (m_indexGeneration.count(index) > 0)
      return EGeneratingIndex;

    LocalAndRemoteSizeT const size = CountryByIndex(index).Size();
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
      // reset total country's download progress
      LocalAndRemoteSizeT const size = CountryByIndex(index).Size();
      m_countryProgress.first = 0;
      m_countryProgress.second = size.second;

      DownloadNextCountryFromQueue();
    }
    else
    {
      // notify about "In Queue" status
      NotifyStatusChanhed(index);
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
    void operator()(CountryFile const & file)
    {
      m_removeFn(file.GetFileWithExt());
    }
  };

  void Storage::NotifyStatusChanhed(TIndex const & index) const
  {
    if (m_observerChange)
      m_observerChange(index);
  }

  void Storage::DownloadNextCountryFromQueue()
  {
    while (!m_queue.empty())
    {
      TIndex index = m_queue.front();
      FilesContainerT const & tiles = CountryByIndex(index).Files();
      for (FilesContainerT::const_iterator it = tiles.begin(); it != tiles.end(); ++it)
      {
        if (!IsFileDownloaded(*it))
        {
          // send Country name for statistics
          string const postBody = it->m_fileName;
          m_request.reset(HttpRequest::PostJson(GetPlatform().MetaServerUrl(),
              postBody,
              bind(&Storage::OnServerListDownloaded, this, _1)));

          // new status for country, "Downloading"
          NotifyStatusChanhed(index);
          return;
        }
      }

      // continue with next country
      m_queue.pop_front();
      // reset total country's download progress
      if (!m_queue.empty())
      {
        m_countryProgress.first = 0;
        m_countryProgress.second = CountryByIndex(m_queue.front()).Size().second;
      }

      // new status for country, "OnDisk"
      NotifyStatusChanhed(index);
    }
  }

  class DeleteMap
  {
    string m_workingDir;
  public:
    DeleteMap()
    {
      m_workingDir = GetPlatform().WritableDir();
    }
    /// @TODO do not delete other countries cells
    void operator()(CountryFile const & file)
    {
      FileWriter::DeleteFileX(m_workingDir + file.m_fileName + DOWNLOADING_FILE_EXTENSION);
      FileWriter::DeleteFileX(m_workingDir + file.m_fileName + RESUME_FILE_EXTENSION);
      FileWriter::DeleteFileX(m_workingDir + file.GetFileWithExt());
    }
  };

  template <typename TRemoveFunc>
  void DeactivateAndDeleteCountry(Country const & country, TRemoveFunc removeFunc)
  {
    // deactivate from multiindex
    for_each(country.Files().begin(), country.Files().end(), DeactivateMap<TRemoveFunc>(removeFunc));
    // delete from disk
    for_each(country.Files().begin(), country.Files().end(), DeleteMap());
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
      {
        // stop download
        m_request.reset();
        // remove from the queue
        m_queue.erase(found);
        // reset progress if the queue is not empty
        if (!m_queue.empty())
        {
          m_countryProgress.first = 0;
          m_countryProgress.second = CountryByIndex(m_queue.front()).Size().second;
        }
        // start another download if the queue is not empty
        DownloadNextCountryFromQueue();
      }
      else
      {
        // remove from the queue
        m_queue.erase(found);
      }
    }
    else
    {
      // bounds are only updated if country was already activated before
      bounds = country.Bounds();
    }

    DeactivateAndDeleteCountry(country, m_removeMap);
    NotifyStatusChanhed(index);

    if (bounds != m2::RectD::GetEmptyRect())
      m_updateRect(bounds);
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

  void Storage::Subscribe(TObserverChangeCountryFunction change,
                          TObserverProgressFunction progress)
  {
    m_observerChange = change;
    m_observerProgress = progress;
  }

  void Storage::Unsubscribe()
  {
    m_observerChange.clear();
    m_observerProgress.clear();
  }

  void Storage::OnMapDownloadFinished(HttpRequest & request)
  {
    if (m_queue.empty())
    {
      ASSERT ( false, ("Invalid url?", request.Data()) );
      return;
    }

    TIndex const index = m_queue.front();
    if (request.Status() == HttpRequest::EFailed)
    {
      // remove failed country from the queue
      m_queue.pop_front();
      m_failedCountries.insert(index);

      // notify GUI about failed country
      NotifyStatusChanhed(index);
    }
    else
    {
      LocalAndRemoteSizeT const size = CountryByIndex(index).Size();
      if (size.second != 0)
        m_countryProgress.first = size.first;

      // get file descriptor
      string file = request.Data();

      // FIXME
      string::size_type const i = file.find_last_of("/\\");
      if (i != string::npos)
        file = file.substr(i+1);

      Platform & pl = GetPlatform();
      if (pl.IsFeatureSupported("search"))
      {
        // Generate search index if it's supported in this build
        m_indexGeneration.insert(index);
        pl.RunAsync(bind(&Storage::GenerateSearchIndex, this, index, file));
      }
      else
      {
        // Or simply activate downloaded map
        UpdateAfterSearchIndex(index, file);
      }
    }

    m_request.reset();
    DownloadNextCountryFromQueue();
  }

  void Storage::GenerateSearchIndex(TIndex const & index, string const & fName)
  {
    if (indexer::BuildSearchIndexFromDatFile(fName))
    {
      GetPlatform().RunOnGuiThread(bind(&Storage::UpdateAfterSearchIndex, this, index, fName));
    }
    else
    {
      LOG(LERROR, ("Can't build search index for", fName));
    }
  }

  void Storage::UpdateAfterSearchIndex(TIndex const & index, string const & fName)
  {
    // remove from index set
    m_indexGeneration.erase(index);
    NotifyStatusChanhed(index);

    // activate downloaded map piece
    m_addMap(fName);

    // update rect from downloaded file
    feature::DataHeader header;
    LoadMapHeader(GetPlatform().GetReader(fName), header);
    m_updateRect(header.GetBounds());
  }

  void Storage::OnMapDownloadProgress(HttpRequest & request)
  {
    if (m_queue.empty())
    {
      ASSERT(false, ("queue can't be empty"));
      return;
    }

    if (m_observerProgress)
    {
      HttpRequest::ProgressT p = request.Progress();
      p.first += m_countryProgress.first;
      p.second = m_countryProgress.second;
      m_observerProgress(m_queue.front(), p);
    }
  }

  void Storage::OnServerListDownloaded(HttpRequest & request)
  {
    if (m_queue.empty())
    {
      ASSERT(false, ("this should never happen"));
      return;
    }

    // @TODO now supports only one file in the country
    CountryFile const & file = CountryByIndex(m_queue.front()).Files().front();

    vector<string> urls;
    if (request.Status() == HttpRequest::ECompleted
        && ParseServerList(request.Data(), urls))
      Settings::Set(SETTINGS_SERVERS_KEY, request.Data());
    else
    {
      string serverList;
      if (!Settings::Get(SETTINGS_SERVERS_KEY, serverList))
        serverList = GetPlatform().DefaultUrlsJSON();
      VERIFY(ParseServerList(serverList, urls), ());
    }

    // append actual version and file name
    for (size_t i = 0; i < urls.size(); ++i)
      urls[i] = urls[i] + OMIM_OS_NAME "/"
          + strings::to_string(m_currentVersion)  + "/" + UrlEncode(file.GetFileWithExt());

    m_request.reset(HttpRequest::GetFile(urls,
                                         GetPlatform().WritablePathForFile(file.GetFileWithExt()),
                                         file.m_remoteSize,
                                         bind(&Storage::OnMapDownloadFinished, this, _1),
                                         bind(&Storage::OnMapDownloadProgress, this, _1)));
  }
}
