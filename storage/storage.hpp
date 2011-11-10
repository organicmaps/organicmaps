#pragma once

#include "../storage/country.hpp"

#include "../platform/http_request.hpp"

#include "../std/vector.hpp"
#include "../std/map.hpp"
#include "../std/list.hpp"
#include "../std/string.hpp"
#include "../std/set.hpp"
#include "../std/function.hpp"
#include "../std/scoped_ptr.hpp"

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
    static int const INVALID;
    int m_group;
    int m_country;
    int m_region;
    TIndex(int group = INVALID, int country = INVALID, int region = INVALID)
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
    /// We support only one simultaneous request at the moment
    scoped_ptr<downloader::HttpRequest> m_request;

    /// stores timestamp for update checks
    int64_t m_currentVersion;

    CountriesContainerT m_countries;

    typedef list<TIndex> TQueue;
    TQueue m_queue;
    /// used to correctly calculate total country download progress with more than 1 file
    /// <current, total>
    downloader::HttpRequest::ProgressT m_countryProgress;

    typedef set<TIndex> TFailedCountries;
    /// stores countries which download has failed recently
    TFailedCountries m_failedCountries;

    /// @name Communicate with GUI
    //@{
    typedef function<void (TIndex const &)> TObserverChangeCountryFunction;
    typedef function<void (TIndex const &, pair<int64_t, int64_t> const &)> TObserverProgressFunction;
    TObserverChangeCountryFunction m_observerChange;
    TObserverProgressFunction m_observerProgress;
    //@}

    /// @name Communicate with Framework
    //@{
    typedef vector<string> map_list_t;
  public:
    typedef function<void (string const &)> TAddMapFunction;
    typedef function<void (string const &)> TRemoveMapFunction;
    typedef function<void (m2::RectD const & r)> TUpdateRectFunction;
    typedef function<void (map_list_t &)> TEnumMapsFunction;

  private:
    TAddMapFunction m_addMap;
    TRemoveMapFunction m_removeMap;
    TUpdateRectFunction m_updateRect;
    //@}

    void DownloadNextCountryFromQueue();
    Country const & CountryByIndex(TIndex const & index) const;
    string UpdateBaseUrl() const;

  public:
    Storage() {}
    /// @TODO temporarily made public for Android, refactor
    void ReInitCountries(bool forceReload);

    void Init(TAddMapFunction addFunc, TRemoveMapFunction removeFunc, TUpdateRectFunction updateRectFunc);

    /// @name Called from DownloadManager
    //@{
    void OnMapDownloadFinished(downloader::HttpRequest & request);
    void OnMapDownloadProgress(downloader::HttpRequest & request);
    //@}

    /// @name Current impl supports only one observer
    //@{
    void Subscribe(TObserverChangeCountryFunction change,
                   TObserverProgressFunction progress);
    void Unsubscribe();
    //@}

    size_t CountriesCount(TIndex const & index) const;
    string const & CountryName(TIndex const & index) const;
    string const & CountryFlag(TIndex const & index) const;
    LocalAndRemoteSizeT CountrySizeInBytes(TIndex const & index) const;
    TStatus CountryStatus(TIndex const & index) const;
    m2::RectD CountryBounds(TIndex const & index) const;

    void DownloadCountry(TIndex const & index);
    void DeleteCountry(TIndex const & index);

    void CheckForUpdate();
  };
}
