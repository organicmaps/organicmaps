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
    EInQueue,
    EUnknown
  };

  struct TIndex
  {
    int m_group;
    int m_country;
    int m_region;
    TIndex(int group = -1, int country = -1, int region = -1)
      : m_group(group), m_country(country), m_region(region) {}
    bool operator==(TIndex const & other) const
    {
      return m_group == other.m_group && m_country == other.m_country && m_region == other.m_region;
    }
  };

  /// Can be used to store local maps and/or maps available for download
  class Storage
  {
    /// stores timestamp for update checks
    uint32_t m_currentVersion;

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
    Country const & CountryByIndex(TIndex const & index) const;
    bool UpdateCheck();
    string UpdateBaseUrl() const;

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

    size_t CountriesCount(TIndex const & index) const;
    string CountryName(TIndex const & index) const;
    TLocalAndRemoteSize CountrySizeInBytes(TIndex const & index) const;
    TStatus CountryStatus(TIndex const & index) const;

    void DownloadCountry(TIndex const & index);
    void DeleteCountry(TIndex const & index);
  };
}
