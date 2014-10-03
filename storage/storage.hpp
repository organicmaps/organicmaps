#pragma once

#include "storage_defines.hpp"
#include "country.hpp"
#include "index.hpp"
#include "guides.hpp"

#include "../platform/http_request.hpp"

#include "../std/vector.hpp"
#include "../std/map.hpp"
#include "../std/list.hpp"
#include "../std/string.hpp"
#include "../std/set.hpp"
#include "../std/function.hpp"
#include "../std/unique_ptr.hpp"

namespace storage
{
  /// Can be used to store local maps and/or maps available for download
  class Storage
  {
    /// We support only one simultaneous request at the moment
    unique_ptr<downloader::HttpRequest> m_request;

    /// stores timestamp for update checks
    int64_t m_currentVersion;

    CountriesContainerT m_countries;

    vector<TIndex> m_downloadedCountries;
    vector<TIndex> m_outOfDateCountries;

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
    typedef function<void (TIndex const &, LocalAndRemoteSizeT const &)> TProgressFunction;

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
    /// @return Country file name without extension.
    string const & CountryFileName(TIndex const & index) const;
    LocalAndRemoteSizeT CountrySizeInBytes(TIndex const & index) const;
    LocalAndRemoteSizeT CountrySizeInBytesEx(TIndex const & index, TMapOptions const & options) const;
    /// Fast version, doesn't check if country is out of date
    TStatus CountryStatus(TIndex const & index) const;
    /// Slow version, but checks if country is out of date
    TStatus CountryStatusEx(TIndex const & index) const;
    void CountryStatusEx(TIndex const & index, TStatus & status, TMapOptions & options) const;
    //m2::RectD CountryBounds(TIndex const & index) const;

    void DownloadCountry(TIndex const & index, TMapOptions const & options);
    bool DeleteFromDownloader(TIndex const & index);
    bool IsDownloadInProgress() const;

    void NotifyStatusChanged(TIndex const & index);

    string GetFileDownloadUrl(string const & baseUrl, string const & fName) const;

    /// @param[out] res Populated with oudated countries.
    void GetOutdatedCountries(vector<Country const *> & res) const;

    int64_t GetCurrentDataVersion() const { return m_currentVersion; }

    //@{
  private:
    guides::GuidesManager m_guideManager;

  public:
    guides::GuidesManager const & GetGuideManager() const { return m_guideManager; }
    guides::GuidesManager & GetGuideManager() { return m_guideManager; }
    //@}
  };
}
