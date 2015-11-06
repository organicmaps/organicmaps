#pragma once

#include "storage/country.hpp"
#include "storage/index.hpp"
#include "storage/map_files_downloader.hpp"
#include "storage/queued_country.hpp"
#include "storage/storage_defines.hpp"

#include "platform/local_country_file.hpp"

#include "std/function.hpp"
#include "std/list.hpp"
#include "std/set.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace storage
{
/// Can be used to store local maps and/or maps available for download
class Storage
{
public:
  using TUpdate = function<void(platform::LocalCountryFile const &)>;

private:
  /// We support only one simultaneous request at the moment
  unique_ptr<MapFilesDownloader> m_downloader;

  /// stores timestamp for update checks
  int64_t m_currentVersion;

  CountriesContainerT m_countries;

  typedef list<QueuedCountry> TQueue;

  /// @todo. It appeared that our application uses m_queue from
  /// different threads without any synchronization. To reproduce it
  /// just download a map "from the map" on Android. (CountryStatus is
  /// called from a different thread.)  It's necessary to check if we
  /// can call all the methods from a single thread using
  /// RunOnUIThread.  If not, at least use a syncronization object.
  TQueue m_queue;

  /// stores countries whose download has failed recently
  typedef set<TIndex> TCountriesSet;
  TCountriesSet m_failedCountries;

  using TLocalFilePtr = shared_ptr<platform::LocalCountryFile>;
  map<TIndex, list<TLocalFilePtr>> m_localFiles;

  // Our World.mwm and WorldCoasts.mwm are fake countries, together with any custom mwm in data
  // folder.
  map<platform::CountryFile, TLocalFilePtr> m_localFilesForFakeCountries;

  /// used to correctly calculate total country download progress with more than 1 file
  /// <current, total>
  MapFilesDownloader::TProgress m_countryProgress;

  /// @name Communicate with GUI
  //@{
  typedef function<void(TIndex const &)> TChangeCountryFunction;
  typedef function<void(TIndex const &, LocalAndRemoteSizeT const &)> TProgressFunction;

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

  // This function is called each time all files requested for a
  // country were successfully downloaded.
  TUpdate m_update;

  void DownloadNextCountryFromQueue();

  void LoadCountriesFile(bool forceReload);

  void ReportProgress(TIndex const & index, pair<int64_t, int64_t> const & p);

  /// Called on the main thread by MapFilesDownloader when list of
  /// suitable servers is received.
  void OnServerListDownloaded(vector<string> const & urls);

  /// Called on the main thread by MapFilesDownloader when
  /// downloading of a map file succeeds/fails.
  void OnMapFileDownloadFinished(bool success, MapFilesDownloader::TProgress const & progress);

  /// Periodically called on the main thread by MapFilesDownloader
  /// during the downloading process.
  void OnMapFileDownloadProgress(MapFilesDownloader::TProgress const & progress);

  bool RegisterDownloadedFiles(TIndex const & index, MapOptions files);
  void OnMapDownloadFinished(TIndex const & index, bool success, MapOptions files);

  /// Initiates downloading of the next file from the queue.
  void DownloadNextFile(QueuedCountry const & country);

public:
  Storage();

  void Init(TUpdate const & update);

  // Clears local files registry and downloader's queue.
  void Clear();

  // Finds and registers all map files in maps directory. In the case
  // of several versions of the same map keeps only the latest one, others
  // are deleted from disk.
  // *NOTE* storage will forget all already known local maps.
  void RegisterAllLocalMaps();

  // Returns list of all local maps, including fake countries (World*.mwm).
  void GetLocalMaps(vector<TLocalFilePtr> & maps) const;
  // Returns number of downloaded maps (files), excluding fake countries (World*.mwm).
  size_t GetDownloadedFilesCount() const;

  /// @return unique identifier that should be used with Unsubscribe function
  int Subscribe(TChangeCountryFunction const & change, TProgressFunction const & progress);
  void Unsubscribe(int slotId);

  Country const & CountryByIndex(TIndex const & index) const;
  TIndex FindIndexByFile(string const & name) const;
  /// @todo Temporary function to gel all associated indexes for the country file name.
  /// Will be removed in future after refactoring.
  vector<TIndex> FindAllIndexesByFile(string const & name) const;
  void GetGroupAndCountry(TIndex const & index, string & group, string & country) const;

  size_t CountriesCount(TIndex const & index) const;
  string const & CountryName(TIndex const & index) const;
  string const & CountryFlag(TIndex const & index) const;

  LocalAndRemoteSizeT CountrySizeInBytes(TIndex const & index, MapOptions opt) const;
  platform::CountryFile const & GetCountryFile(TIndex const & index) const;
  TLocalFilePtr GetLatestLocalFile(platform::CountryFile const & countryFile) const;
  TLocalFilePtr GetLatestLocalFile(TIndex const & index) const;

  /// Fast version, doesn't check if country is out of date
  TStatus CountryStatus(TIndex const & index) const;
  /// Slow version, but checks if country is out of date
  TStatus CountryStatusEx(TIndex const & index) const;
  void CountryStatusEx(TIndex const & index, TStatus & status, MapOptions & options) const;

  /// Puts country denoted by index into the downloader's queue.
  /// During downloading process notifies observers about downloading
  /// progress and status changes.
  void DownloadCountry(TIndex const & index, MapOptions opt);

  /// Removes country files (for all versions) from the device.
  /// Notifies observers about country status change.
  void DeleteCountry(TIndex const & index, MapOptions opt);

  /// Removes country files of a particular version from the device.
  /// Notifies observers about country status change.
  void DeleteCustomCountryVersion(platform::LocalCountryFile const & localFile);

  /// \return True iff country denoted by index was successfully
  ///          deleted from the downloader's queue.
  bool DeleteFromDownloader(TIndex const & index);
  bool IsDownloadInProgress() const;

  TIndex GetCurrentDownloadingCountryIndex() const;

  void NotifyStatusChanged(TIndex const & index);

  /// get download url by index & options(first search file name by index, then format url)
  string GetFileDownloadUrl(string const & baseUrl, TIndex const & index, MapOptions file) const;
  /// get download url by base url & file name
  string GetFileDownloadUrl(string const & baseUrl, string const & fName) const;

  /// @param[out] res Populated with oudated countries.
  void GetOutdatedCountries(vector<Country const *> & countries) const;

  inline int64_t GetCurrentDataVersion() const { return m_currentVersion; }

  void SetDownloaderForTesting(unique_ptr<MapFilesDownloader> && downloader);
  void SetCurrentDataVersionForTesting(int64_t currentVersion);

private:
  friend void UnitTest_StorageTest_DeleteCountry();

  TStatus CountryStatusWithoutFailed(TIndex const & index) const;
  TStatus CountryStatusFull(TIndex const & index, TStatus const status) const;

  // Modifies file set of requested files - always adds a map file
  // when routing file is requested for downloading, but drops all
  // already downloaded and up-to-date files.
  MapOptions NormalizeDownloadFileSet(TIndex const & index, MapOptions options) const;

  // Modifies file set of file to deletion - always adds (marks for
  // removal) a routing file when map file is marked for deletion.
  MapOptions NormalizeDeleteFileSet(MapOptions options) const;

  // Returns a pointer to a country in the downloader's queue.
  QueuedCountry * FindCountryInQueue(TIndex const & index);

  // Returns a pointer to a country in the downloader's queue.
  QueuedCountry const * FindCountryInQueue(TIndex const & index) const;

  // Returns true when country is in the downloader's queue.
  bool IsCountryInQueue(TIndex const & index) const;

  // Returns true when country is first in the downloader's queue.
  bool IsCountryFirstInQueue(TIndex const & index) const;

  // Returns local country files of a particular version, or wrapped
  // nullptr if there're no country files corresponding to the
  // version.
  TLocalFilePtr GetLocalFile(TIndex const & index, int64_t version) const;

  // Tries to register disk files for a real (listed in countries.txt)
  // country. If map files of the same version were already
  // registered, does nothing.
  void RegisterCountryFiles(TLocalFilePtr localFile);

  // Registers disk files for a country. This method must be used only
  // for real (listed in counties.txt) countries.
  void RegisterCountryFiles(TIndex const & index, string const & directory, int64_t version);

  // Registers disk files for a country. This method must be used only
  // for custom (made by user) map files.
  void RegisterFakeCountryFiles(platform::LocalCountryFile const & localFile);

  // Removes disk files for all versions of a country.
  void DeleteCountryFiles(TIndex const & index, MapOptions opt);

  // Removes country files from downloader.
  bool DeleteCountryFilesFromDownloader(TIndex const & index, MapOptions opt);

  // Returns download size of the currently downloading file for the
  // queued country.
  uint64_t GetDownloadSize(QueuedCountry const & queuedCountry) const;

  // Returns a path to a place on disk downloader can use for
  // downloaded files.
  string GetFileDownloadPath(TIndex const & index, MapOptions file) const;
};
}  // storage
