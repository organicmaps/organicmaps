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
    EOnDisk = 0,
    ENotDownloaded,
    EDownloadFailed,
    EDownloading,
    EInQueue,
    EUnknown,
    EGeneratingIndex
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
      return (m_group == other.m_group &&
              m_country == other.m_country &&
              m_region == other.m_region);
    }

    bool operator!=(TIndex const & other) const
    {
      return !(*this == other);
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

  string DebugPrint(TIndex const & r);

  /// Can be used to store local maps and/or maps available for download
  class Storage
  {
    /// We support only one simultaneous request at the moment
    scoped_ptr<downloader::HttpRequest> m_request;

    /// stores timestamp for update checks
    int64_t m_currentVersion;

    CountriesContainerT m_countries;

    /// store queue for downloading
    typedef list<TIndex> TQueue;
    TQueue m_queue;

    /// stores countries which download has failed recently
    typedef set<TIndex> TCountriesSet;
    TCountriesSet m_failedCountries;

    /// store countries set for which search index is generating
    TCountriesSet m_indexGeneration;

    /// used to correctly calculate total country download progress with more than 1 file
    /// <current, total>
    downloader::HttpRequest::ProgressT m_countryProgress;

    /// @name Communicate with GUI
    //@{
    typedef function<void (TIndex const &)> TChangeCountryFunction;
    typedef function<void (TIndex const &, pair<int64_t, int64_t> const &)> TProgressFunction;

    int m_currentSlotId;

    struct CountryObservers
    {
      TChangeCountryFunction m_changeCountryFn;
      TProgressFunction m_progressFn;
      int m_slotId;
    };

    list<CountryObservers> m_observers;
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

    //void GenerateSearchIndex(TIndex const & index, string const & fName);
    void UpdateAfterSearchIndex(TIndex const & index, string const & fName);

    /// @TODO temporarily made public for Android, refactor
    void LoadCountriesFile(bool forceReload);

    void ReportProgress(TIndex const & index, pair<int64_t, int64_t> const & p);

  public:
    Storage();

    void Init(TAddMapFunction addFunc,
              TRemoveMapFunction removeFunc,
              TUpdateRectFunction updateRectFunc);

    /// @name Called from DownloadManager
    //@{
    void OnServerListDownloaded(downloader::HttpRequest & request);
    void OnMapDownloadFinished(downloader::HttpRequest & request);
    void OnMapDownloadProgress(downloader::HttpRequest & request);
    //@}

    /// @name Current impl supports only one observer
    //@{

    /// @return unique identifier that should be used with Unsubscribe function
    int Subscribe(TChangeCountryFunction change,
                  TProgressFunction progress);
    void Unsubscribe(int slotId);
    //@}

    TIndex const FindIndexByName(string const & name) const;

    size_t CountriesCount(TIndex const & index) const;
    string const & CountryName(TIndex const & index) const;
    string const & CountryFlag(TIndex const & index) const;
    LocalAndRemoteSizeT CountrySizeInBytes(TIndex const & index) const;
    TStatus CountryStatus(TIndex const & index) const;
    m2::RectD CountryBounds(TIndex const & index) const;

    void DownloadCountry(TIndex const & index);
    void DeleteCountry(TIndex const & index);

    void CheckForUpdate();

    void NotifyStatusChanged(TIndex const & index) const;

    string GetFileDownloadUrl(string const & baseUrl, string const & fName) const;
  };
}
