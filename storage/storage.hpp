#pragma once

#include "../platform/download_manager.hpp"
#include "../platform/platform.hpp"

#include "../defines.hpp"
#include "../storage/country.hpp"

#include "../std/vector.hpp"
#include "../std/map.hpp"
#include "../std/list.hpp"
#include "../std/string.hpp"
#include "../std/set.hpp"

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

  enum TUpdateResult
  {
    ENoAnyUpdateAvailable = 0,
    ENewBinaryAvailable = 0x01,
    EBinaryCheckFailed = 0x02,
    EBinaryUpdateFailed = 0x04,
    ENewDataAvailable = 0x08,
    EDataCheckFailed = 0x10,
    EDataUpdateFailed = 0x20
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
    bool operator<(TIndex const & other) const
    {
      if (m_group != other.m_group)
        return m_group < other.m_group;
      else if (m_country != other.m_country)
        return m_country < other.m_country;
      return m_region < other.m_region;
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

    typedef set<TIndex> TFailedCountries;
    /// stores countries which download has failed recently
    TFailedCountries m_failedCountries;

    /// @name Communicate with GUI
    //@{
    typedef boost::function<void (TIndex const &)> TObserverChangeCountryFunction;
    typedef boost::function<void (TIndex const &, TDownloadProgress const &)> TObserverProgressFunction;
    typedef boost::function<void (TUpdateResult, string const &)> TUpdateRequestFunction;
    TObserverChangeCountryFunction m_observerChange;
    TObserverProgressFunction m_observerProgress;
    TUpdateRequestFunction m_observerUpdateRequest;
    //@}

    /// @name Communicate with Framework
    //@{
    typedef boost::function<void (string const &)> TAddMapFunction;
    typedef boost::function<void (string const &)> TRemoveMapFunction;
    TAddMapFunction m_addMap;
    TRemoveMapFunction m_removeMap;
    //@}

    void DownloadNextCountryFromQueue();
    Country const & CountryByIndex(TIndex const & index) const;
    string UpdateBaseUrl() const;
    void ReInitCountries(bool forceReload);

  public:
    Storage() {}

    /// Adds all locally downloaded maps to the model
    void Init(TAddMapFunction addFunc, TRemoveMapFunction removeFunc);

    /// @name Called from DownloadManager
    //@{
    void OnMapDownloadFinished(char const * url, DownloadResult result);
    void OnMapDownloadProgress(char const * url, TDownloadProgress progress);
    void OnDataUpdateCheckFinished(char const * url, DownloadResult result);
    void OnBinaryUpdateCheckFinished(char const * url, DownloadResult result);
    //@}

    /// @name Current impl supports only one observer
    //@{
    void Subscribe(TObserverChangeCountryFunction change,
                   TObserverProgressFunction progress,
                   TUpdateRequestFunction dataCheck);
    void Unsubscribe();
    //@}

    size_t CountriesCount(TIndex const & index) const;
    string CountryName(TIndex const & index) const;
    TLocalAndRemoteSize CountrySizeInBytes(TIndex const & index) const;
    TStatus CountryStatus(TIndex const & index) const;

    void DownloadCountry(TIndex const & index);
    void DeleteCountry(TIndex const & index);

    void CheckForUpdate();
  };
}
