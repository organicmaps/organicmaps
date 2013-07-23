#pragma once

#include "country.hpp"
#include "index.hpp"

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
    EOnDiskOutOfDate
  };

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

    typedef list<CountryObservers> ObserversContT;
    ObserversContT m_observers;
    //@}

    /// @name Communicate with Framework
    //@{
    typedef function<void (string const &)> TUpdateAfterDownload;
    TUpdateAfterDownload m_updateAfterDownload;
    //@}

    void DownloadNextCountryFromQueue();

    void LoadCountriesFile(bool forceReload);

    void ReportProgress(TIndex const & index, pair<int64_t, int64_t> const & p);

  public:
    Storage();

    void Init(TUpdateAfterDownload const & updateFn);

    /// @name Called from DownloadManager
    //@{
    void OnServerListDownloaded(downloader::HttpRequest & request);
    void OnMapDownloadFinished(downloader::HttpRequest & request);
    void OnMapDownloadProgress(downloader::HttpRequest & request);
    //@}

    /// @name Current impl supports only one observer
    //@{

    /// @return unique identifier that should be used with Unsubscribe function
    int Subscribe(TChangeCountryFunction const & change,
                  TProgressFunction const & progress);
    void Unsubscribe(int slotId);
    //@}

    Country const & CountryByIndex(TIndex const & index) const;
    TIndex FindIndexByFile(string const & name) const;
    void GetGroupAndCountry(TIndex const & index, string & group, string & country) const;

    size_t CountriesCount(TIndex const & index) const;
    string const & CountryName(TIndex const & index) const;
    string const & CountryFlag(TIndex const & index) const;
    LocalAndRemoteSizeT CountrySizeInBytes(TIndex const & index) const;
    TStatus CountryStatus(TIndex const & index) const;
    //m2::RectD CountryBounds(TIndex const & index) const;

    void DownloadCountry(TIndex const & index);
    bool DeleteFromDownloader(TIndex const & index);
    bool IsDownloadInProgress() const;

    void NotifyStatusChanged(TIndex const & index) const;

    string GetFileDownloadUrl(string const & baseUrl, string const & fName) const;

    /// @param list is populated with oudated countries
    /// @return number of outdated countries in the list
    int GetOutdatedCountries(vector<Country> & list);

    int64_t GetCurrentDataVersion();
  };
}
