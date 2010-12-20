#pragma once

#include "../platform/download_manager.hpp"
#include "../platform/platform.hpp"

#include "../storage/defines.hpp"
#include "../storage/country.hpp"

#include "../std/vector.hpp"
#include "../std/map.hpp"
#include "../std/list.hpp"
#include "../std/string.hpp"

#include <boost/function.hpp>

namespace storage
{
  /// Used in GUI
  enum TStatus
  {
    EOnDisk,
    ENotDownloaded,
    EDownloadFailed,
    EDownloading,
    EInQueue
  };

  typedef std::pair<size_t, size_t> TIndex;

  /// Can be used to store local maps and/or maps available for download
  class Storage
  {
    TCountriesContainer m_countries;

    typedef list<TIndex> TQueue;
    TQueue m_queue;
    /// used to correctly calculate total country download progress
    TDownloadProgress m_countryProgress;

    /// @name Communicate with GUI
    //@{
    typedef boost::function<void (TIndex const &)> TObserverChangeCountryFunction;
    typedef boost::function<void (TIndex const &, TDownloadProgress const &)> TObserverProgressFunction;
    TObserverChangeCountryFunction m_observerChange;
    TObserverProgressFunction m_observerProgress;
    //@}

    /// @name Communicate with Framework
    //@{
    typedef boost::function<void (string const &, string const &)> TAddMapFunction;
    typedef boost::function<void (string const &)> TRemoveMapFunction;
    TAddMapFunction m_addMap;
    TRemoveMapFunction m_removeMap;
    //@}

    void DownloadNextCountryFromQueue();
    Country const * CountryByIndex(TIndex const & index) const;
    bool UpdateCheck();

  public:
    Storage() {}

    /// Adds all locally downloaded maps to the model
    void Init(TAddMapFunction addFunc, TRemoveMapFunction removeFunc);

    /// @name Called from DownloadManager
    //@{
    void OnMapDownloadFinished(char const * url, bool successfully);
    void OnMapDownloadProgress(char const * url, TDownloadProgress progress);
    void OnUpdateDownloadFinished(char const * url, bool successfully);
    //@}

    /// @name Current impl supports only one observer
    //@{
    void Subscribe(TObserverChangeCountryFunction change, TObserverProgressFunction progress);
    void Unsubscribe();
    //@}

    size_t GroupsCount() const;
    size_t CountriesCountInGroup(size_t groupIndex) const;
    string GroupName(size_t groupIndex) const;
    string CountryName(TIndex const & index) const;
    uint64_t CountrySizeInBytes(TIndex const & index) const;
    TStatus CountryStatus(TIndex const & index) const;

    void DownloadCountry(TIndex const & index);
    void DeleteCountry(TIndex const & index);
  };
}
