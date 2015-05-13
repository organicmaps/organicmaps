#pragma once

#include "storage/storage_defines.hpp"
#include "storage/country.hpp"
#include "storage/index.hpp"

#include "platform/http_request.hpp"

#include "std/vector.hpp"
#include "std/list.hpp"
#include "std/string.hpp"
#include "std/set.hpp"
#include "std/function.hpp"
#include "std/unique_ptr.hpp"


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

    /// store queue for downloading
    class QueuedCountry
    {
      TIndex m_index;
      CountryFile const * m_pFile;
      TMapOptions m_init, m_left, m_current;

    public:
      QueuedCountry(Storage const & storage, TIndex const & index, TMapOptions opt);

      void AddOptions(TMapOptions opt);
      bool MoveNextFile();
      bool Correct(TStatus currentStatus);

      TIndex const & GetIndex() const { return m_index; }
      TMapOptions GetInitOptions() const { return m_init; }

      bool operator== (TIndex const & index) const { return (m_index == index); }

      uint64_t GetDownloadSize() const;
      LocalAndRemoteSizeT GetFullSize() const;
      size_t GetFullRemoteSize() const;
      string GetFileName() const;
      string GetMapFileName() const;
    };

    typedef list<QueuedCountry> TQueue;
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
    typedef function<void (string const &, TMapOptions)> TUpdateAfterDownload;
    TUpdateAfterDownload m_updateAfterDownload;
    //@}

    void DownloadNextCountryFromQueue();

    void LoadCountriesFile(bool forceReload);

    void ReportProgress(TIndex const & index, pair<int64_t, int64_t> const & p);

    /// @name
    //@{
    void OnServerListDownloaded(downloader::HttpRequest & request);
    void OnMapDownloadFinished(downloader::HttpRequest & request);
    void OnMapDownloadProgress(downloader::HttpRequest & request);
    void DownloadNextFile(QueuedCountry const & cnt);
    //@}

  public:
    Storage();

    void Init(TUpdateAfterDownload const & updateFn);

    /// @return unique identifier that should be used with Unsubscribe function
    int Subscribe(TChangeCountryFunction const & change,
                  TProgressFunction const & progress);
    void Unsubscribe(int slotId);

    Country const & CountryByIndex(TIndex const & index) const;
    TIndex FindIndexByFile(string const & name) const;
    void GetGroupAndCountry(TIndex const & index, string & group, string & country) const;

    size_t CountriesCount(TIndex const & index) const;
    string const & CountryName(TIndex const & index) const;
    string const & CountryFlag(TIndex const & index) const;

    string CountryFileName(TIndex const & index, TMapOptions opt) const;
    string const & CountryFileNameWithoutExt(TIndex const & index) const;
    /// Removes map file extension.
    static string MapWithoutExt(string mapFile);
    LocalAndRemoteSizeT CountrySizeInBytes(TIndex const & index, TMapOptions opt) const;

    /// Fast version, doesn't check if country is out of date
    TStatus CountryStatus(TIndex const & index) const;
    /// Slow version, but checks if country is out of date
    TStatus CountryStatusEx(TIndex const & index) const;
    void CountryStatusEx(TIndex const & index, TStatus & status, TMapOptions & options) const;

    //m2::RectD CountryBounds(TIndex const & index) const;

    void DownloadCountry(TIndex const & index, TMapOptions opt);
    bool DeleteFromDownloader(TIndex const & index);
    bool IsDownloadInProgress() const;

    void NotifyStatusChanged(TIndex const & index);

    string GetFileDownloadUrl(string const & baseUrl, string const & fName) const;

    /// @param[out] res Populated with oudated countries.
    void GetOutdatedCountries(vector<Country const *> & res) const;

    int64_t GetCurrentDataVersion() const { return m_currentVersion; }

  private:
    TStatus CountryStatusWithoutFailed(TIndex const & index) const;
    TStatus CountryStatusFull(TIndex const & index, TStatus const status) const;
  };
}
