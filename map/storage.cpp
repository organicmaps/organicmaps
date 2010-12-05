#include "storage.hpp"

#include "../base/logging.hpp"

#include "../coding/file_writer.hpp"
#include "../coding/file_reader.hpp"

#include "../std/set.hpp"
#include "../std/algorithm.hpp"

#include <boost/bind.hpp>

#include "../base/start_mem_debug.hpp"

namespace mapinfo
{
  static bool IsMapValid(string const & datFile)
  {
    // @TODO add more serious integrity checks
    string const indexFile = IndexFileForDatFile(datFile);
    uint64_t datSize = 0, idxSize = 0;
    bool result = GetPlatform().GetFileSize(datFile, datSize)
        && GetPlatform().GetFileSize(indexFile, idxSize);
    if (!result || datSize == 0 || idxSize == 0)
    {
      LOG(LINFO, ("Invalid map files:", datFile, indexFile));
      return false;
    }
    return true;
  }

  /// Scan and load all existing local maps, calculate sum of maps' rects,
  /// also deletes invalid or partially downloaded maps
  template <class TAddFn>
  class AddMapsToModelAndDeleteIfInvalid
  {
    TAddFn & m_addFunc;
    size_t & m_addedCount;
  public:
    AddMapsToModelAndDeleteIfInvalid(TAddFn & addFunc, size_t & addedCount)
      : m_addFunc(addFunc), m_addedCount(addedCount) {}
    /// @return true if map is ok and false if it's deleted or doesn't exist
    bool operator()(string const & datFile)
    {
      if (IsMapValid(datFile))
      {
        m_addFunc(datFile, IndexFileForDatFile(datFile));
        ++m_addedCount;
        return true;
      }
      else
      {
        // looks like map files are bad... delete them
        FileWriter::DeleteFile(datFile);
        FileWriter::DeleteFile(IndexFileForDatFile(datFile));
        return false;
      }
    }
  };

  /// Operates with map files pairs (currently dat and idx)
  template <typename TFunctor>
  void ForEachMapInDir(string const & dir, TFunctor & func)
  {
    Platform::FilesList fileList;
    GetPlatform().GetFilesInDir(dir, "*" DATA_FILE_EXTENSION, fileList);
    std::for_each(fileList.begin(), fileList.end(),
        boost::bind(&TFunctor::operator(), &func,
            boost::bind(std::plus<std::string>(), dir, _1)));
  }

  template <class TAddFn>
  void RescanAndAddDownloadedMaps(TAddFn & addFunc)
  {
    // scan and load all local maps
    size_t addedMapsCount = 0;
    AddMapsToModelAndDeleteIfInvalid<TAddFn> functor(addFunc, addedMapsCount);
    ForEachMapInDir(GetPlatform().WorkingDir(), functor);
    // scan resources if no maps found in data folder
    if (addedMapsCount == 0)
    {
      ForEachMapInDir(GetPlatform().ResourcesDir(), functor);
      if (addedMapsCount == 0)
      {
        LOG(LWARNING, ("No data files were found. Model is empty."));
      }
    }
  }

  void Storage::Init(TAddMapFunction addFunc, TRemoveMapFunction removeFunc)
  {
    m_addMap = addFunc;
    m_removeMap = removeFunc;

    UpdateCheck();

    RescanAndAddDownloadedMaps(addFunc);
  }

  bool Storage::UpdateCheck()
  {
    GetDownloadManager().DownloadFile(
        UPDATE_FULL_URL,
        (GetPlatform().WorkingDir() + UPDATE_CHECK_FILE).c_str(),
        boost::bind(&Storage::OnUpdateDownloadFinished, this, _1, _2),
        TDownloadProgressFunction());
    return true;
  }

  Country const * Storage::CountryByIndex(TIndex const & index) const
  {
    TIndex::first_type i = 0;
    for (TCountriesContainer::const_iterator itGroup = m_countries.begin();
        itGroup != m_countries.end(); ++itGroup, ++i)
    {
      if (i == index.first)
      {
        if (itGroup->second.size() > index.second)
          return &(itGroup->second[index.second]);
        break;
      }
    }
    return 0; // not found
  }

  size_t Storage::GroupsCount() const
  {
    return m_countries.size();
  }

  size_t Storage::CountriesCountInGroup(size_t groupIndex) const
  {
    for (TCountriesContainer::const_iterator it = m_countries.begin(); it != m_countries.end(); ++it)
    {
      if (groupIndex == 0)
        return it->second.size();
      else
        --groupIndex;
    }
    return 0;
  }

  string Storage::GroupName(size_t groupIndex) const
  {
    for (TCountriesContainer::const_iterator it = m_countries.begin(); it != m_countries.end(); ++it)
    {
      if (groupIndex == 0)
        return it->first;
      else
        --groupIndex;
    }
    return string("Atlantida");
  }

  string Storage::CountryName(TIndex const & index) const
  {
    Country const * country = CountryByIndex(index);
    if (country)
      return country->Name();
    else
    {
      ASSERT(false, ("Invalid country index", index));
    }
    return string("Golden Land");
  }

  uint64_t Storage::CountrySizeInBytes(TIndex const & index) const
  {
    Country const * country = CountryByIndex(index);
    if (country)
    {
      uint64_t remoteSize = country->RemoteSize();
      return remoteSize ? remoteSize : country->LocalSize();
    }
    else
    {
      ASSERT(false, ("Invalid country index", index));
    }
    return 0;
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

    Country const * country = CountryByIndex(index);
    if (country)
    {
      if (country->RemoteSize() == 0)
        return EOnDisk;
    }
    else
    {
      ASSERT(false, ("Invalid country index", index));
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
    // otherwise add it into the queue
    m_queue.push_back(index);
    // and start download if necessary
    if (m_queue.size() == 1)
    {
      Country const * country = CountryByIndex(index);
      if (country)
      { // reset total country's download progress
        m_countryProgress = TDownloadProgress(0, country->RemoteSize());
      }

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
      m_workingDir = GetPlatform().WorkingDir();
    }
    void operator()(TUrl const & url)
    {
      string const file = m_workingDir + FileNameFromUrl(url.first);
      if (IsDatFile(file))
        m_removeFn(file);
    }
  };

  void Storage::DownloadNextCountryFromQueue()
  {
    while (!m_queue.empty())
    {
      TIndex index = m_queue.front();
      Country const * country = CountryByIndex(index);
      if (country)
      {
        for (TUrlContainer::const_iterator it = country->Urls().begin(); it != country->Urls().end(); ++it)
        {
          if (!IsFileDownloaded(*it))
          {
            GetDownloadManager().DownloadFile(
                it->first.c_str(),
                (GetPlatform().WorkingDir() + mapinfo::FileNameFromUrl(it->first)).c_str(),
                boost::bind(&Storage::OnMapDownloadFinished, this, _1, _2),
                boost::bind(&Storage::OnMapDownloadProgress, this, _1, _2));
            // notify GUI - new status for country, "Downloading"
            if (m_observerChange)
              m_observerChange(index);
            return;
          }
        }
      }
      // continue with next country
      m_queue.pop_front();
      // reset total country's download progress
      if (!m_queue.empty() && (country = CountryByIndex(m_queue.front())))
        m_countryProgress = TDownloadProgress(0, country->RemoteSize());
      // and notify GUI - new status for country, "OnDisk"
      if (m_observerChange)
        m_observerChange(index);
    }
  }

  struct CancelDownloading
  {
    void operator()(TUrl const & url)
    {
      GetDownloadManager().CancelDownload(url.first.c_str());
      FileWriter::DeleteFile(GetPlatform().WorkingDir() + FileNameFromUrl(url.first));
    }
  };

  void CancelCountryDownload(Country const & country)
  {
    for_each(country.Urls().begin(), country.Urls().end(), CancelDownloading());
  }

  class DeleteMap
  {
    string m_workingDir;
  public:
    DeleteMap()
    {
      m_workingDir = GetPlatform().WorkingDir();
    }
    void operator()(TUrl const & url)
    {
      string const file = m_workingDir + FileNameFromUrl(url.first);
      FileWriter::DeleteFile(file);
    }
  };

  template <typename TRemoveFunc>
  void DeactivateAndDeleteCountry(Country const & country, TRemoveFunc removeFunc)
  {
    // deactivate from multiindex
    for_each(country.Urls().begin(), country.Urls().end(), DeactivateMap<TRemoveFunc>(removeFunc));
    // delete from disk
    for_each(country.Urls().begin(), country.Urls().end(), DeleteMap());
  }

  void Storage::DeleteCountry(TIndex const & index)
  {
    Country const * country = CountryByIndex(index);
    if (!country)
    {
      ASSERT(false, ("Invalid country index"));
      return;
    }
    // check if we already downloading this country
    TQueue::iterator found = find(m_queue.begin(), m_queue.end(), index);
    if (found != m_queue.end())
    {
      if (found == m_queue.begin())
      { // stop download
        CancelCountryDownload(*country);
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

    // @TODO: Do not delete pieces which are used by other countries
    DeactivateAndDeleteCountry(*country, m_removeMap);
    if (m_observerChange)
      m_observerChange(index);
  }

  void Storage::Subscribe(TObserverChangeCountryFunction change, TObserverProgressFunction progress)
  {
    m_observerChange = change;
    m_observerProgress = progress;
  }

  void Storage::Unsubscribe()
  {
    m_observerChange.clear();
    m_observerProgress.clear();
  }

  bool IsUrlValidForCountry(char const * url, Country const * country)
  {
    // additional debug checking
    if (country)
    {
      for (TUrlContainer::const_iterator it = country->Urls().begin(); it != country->Urls().end(); ++it)
        if (it->first == url)
          return true;
    }
    return false;
  }

  void Storage::OnMapDownloadFinished(char const * url, bool successfully)
  {
    Country const * country = 0;
    if (m_queue.empty() || !(country = CountryByIndex(m_queue.front())))
    {
      ASSERT(false, ("Invalid url?", url));
      return;
    }
    ASSERT(IsUrlValidForCountry(url, country), ());
    if (!successfully)
    {
      // remove failed country from the queue
      TIndex failedIndex = m_queue.front();
      m_queue.pop_front();
      // notify GUI about failed country
      if (m_observerChange)
        m_observerChange(failedIndex);
    }
    else
    {
      uint64_t rSize = country->RemoteSize();
      if (rSize == 0)
      { // country is fully downloaded
        // activate it!
        // @TODO: activate only downloaded map, not all maps
        RescanAndAddDownloadedMaps(m_addMap);
      }
      else
      {
        m_countryProgress.first = (m_countryProgress.second - rSize);
      }
    }
    DownloadNextCountryFromQueue();
  }

  void Storage::OnMapDownloadProgress(char const * /*url*/, TDownloadProgress progress)
  {
    if (m_queue.empty())
    {
      ASSERT(false, ("queue can't be empty"));
      return;
    }

    if (m_observerProgress)
      m_observerProgress(m_queue.front(),
          TDownloadProgress(m_countryProgress.first + progress.first, m_countryProgress.second));
  }

  void Storage::OnUpdateDownloadFinished(char const * url, bool successfully)
  {
    if (!successfully)
    {
      LOG(LWARNING, ("Update check failed for url:", url));
      return;
    }

    // parse update file
    TCountriesContainer tempCountries;
    if (!LoadCountries(tempCountries, GetPlatform().WorkingDir() + UPDATE_CHECK_FILE))
    {
      LOG(LWARNING, ("New application version should be downloaded, "
                     "update file format can't be parsed"));
      // @TODO: report to GUI
      return;
    }
    // stop any active download, clear the queue, replace countries and notify GUI
    if (!m_queue.empty())
    {
      Country const * country = CountryByIndex(m_queue.front());
      CancelCountryDownload(*country);
      m_queue.clear();
    }
    m_countries.swap(tempCountries);
    // @TODO report to GUI about reloading all countries
    LOG(LINFO, ("Update check complete"));
  }
}
