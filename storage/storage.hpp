#pragma once

#include "storage/country.hpp"
#include "storage/index.hpp"
#include "storage/map_files_downloader.hpp"
#include "storage/queued_country.hpp"
#include "storage/storage_defines.hpp"

#include "platform/local_country_file.hpp"

#include "std/function.hpp"
#include "std/list.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace storage
{
/// \brief Contains all properties for a node in the country tree.
/// It's applicable for expandable and not expandable node id.
struct NodeAttrs
{
  NodeAttrs() : m_mwmCounter(0), m_localMwmCounter(0), m_mwmSize(0), m_localMwmSize(0),
    m_downloadingMwmSize(0), m_downloadingProgress(0),
    m_status(NodeStatus::Undefined), m_error(NodeErrorCode::NoError) {}
  /// If the node is expandable (a big country) |m_mwmCounter| is number of mwm files (leaves)
  /// belongs to the node. If the node isn't expandable |m_mwmCounter| == 1.
  /// Note. For every expandable node |m_mwmCounter| >= 2.
  uint32_t m_mwmCounter;

  /// Number of mwms belonging to the node which has been donwloaded.
  uint32_t m_localMwmCounter;

  /// If it's not an expandable node, |m_nodeSize| is size of one mwm according to countries.txt.
  /// Otherwise |m_nodeSize| is the sum of all mwm file sizes which belong to the group
  /// according to countries.txt.
  size_t m_mwmSize;

  /// If it's not an expandable node, |m_localMwmSize| is size of one downloaded mwm.
  /// Otherwise |m_localNodeSize| is the sum of all mwm file sizes which belong to the group and
  /// have been downloaded.
  size_t m_localMwmSize;

  /// If downloading or updating an mwm is in progress apart from a local mwm
  /// which is currently used there's a partly downloading mwm of the same region.
  /// |m_downloadingMwmSize| is size of partly downloaded mwm if downloading is in progress.
  /// And |m_downloadingMwmSize| == 0 otherwise.
  size_t m_downloadingMwmSize;

  /// The name of the node in a local language. That means the language dependent on
  /// a device locale.
  string m_nodeLocalName;

  /// The name of the parent in a local language. That means the language dependent on
  /// a device locale. For countries and for the root m_parentLocalName == "".
  string m_parentLocalName;

  /// Node id of the parent of the node. For the root m_parentLocalName == "".
  TCountryId m_parentCountryId;

  /// A number for 0 to 99. It reflects downloading progress in case of
  /// downloading and updating mwm. If downloading or updating is not in progress
  /// |m_downloadingProgress| == 0.
  uint8_t m_downloadingProgress;

  NodeStatus m_status;
  NodeErrorCode m_error;
};

/// This class is used for downloading, updating and deleting maps.
class Storage
{
public:
  struct StatusCallback;
  using TUpdate = function<void(platform::LocalCountryFile const &)>;
  using TChangeCountryFunction = function<void(TCountryId const &)>;
  using TProgressFunction = function<void(TCountryId const &, TLocalAndRemoteSize const &)>;
  using TForEachFunction = function<void(TCountryId const & /* descendantId */, bool /* expandableNode */)>;

private:
  /// We support only one simultaneous request at the moment
  unique_ptr<MapFilesDownloader> m_downloader;

  /// stores timestamp for update checks
  int64_t m_currentVersion;

  TCountriesContainer m_countries;

  using TQueue = list<QueuedCountry>;

  /// @todo. It appeared that our application uses m_queue from
  /// different threads without any synchronization. To reproduce it
  /// just download a map "from the map" on Android. (CountryStatus is
  /// called from a different thread.)  It's necessary to check if we
  /// can call all the methods from a single thread using
  /// RunOnUIThread.  If not, at least use a syncronization object.
  TQueue m_queue;

  /// stores countries whose download has failed recently
  TCountriesSet m_failedCountries;

  using TLocalFilePtr = shared_ptr<platform::LocalCountryFile>;
  map<TCountryId, list<TLocalFilePtr>> m_localFiles;

  // Our World.mwm and WorldCoasts.mwm are fake countries, together with any custom mwm in data
  // folder.
  map<platform::CountryFile, TLocalFilePtr> m_localFilesForFakeCountries;

  /// used to correctly calculate total country download progress with more than 1 file
  /// <current, total>
  MapFilesDownloader::TProgress m_countryProgress;

  /// @name Communicate with GUI
  //@{

  int m_currentSlotId;

  list<StatusCallback> m_statusCallbacks;

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

  // If |m_dataDir| is not empty Storage will create version directories and download maps in
  // platform::WritableDir/|m_dataDir|/. Not empty |m_dataDir| can be used only for
  // downloading maps to a special place but not for continue working with them from this place.
  string m_dataDir;

  // A list of urls for downloading maps. It's necessary for storage integration tests.
  // For example {"http://new-search.mapswithme.com/"}.
  vector<string> m_downloadingUrlsForTesting;

  // |m_downloadMapOnTheMap| is called when an end user clicks on download map or retry button
  // on the map.
  TDownloadFn m_downloadMapOnTheMap;

  void DownloadNextCountryFromQueue();

  void LoadCountriesFile(string const & pathToCountriesFile,
                         string const & dataDir, TMapping * mapping = nullptr);

  void ReportProgress(TCountryId const & countryId, pair<int64_t, int64_t> const & p);

  /// Called on the main thread by MapFilesDownloader when list of
  /// suitable servers is received.
  void OnServerListDownloaded(vector<string> const & urls);

  /// Called on the main thread by MapFilesDownloader when
  /// downloading of a map file succeeds/fails.
  void OnMapFileDownloadFinished(bool success, MapFilesDownloader::TProgress const & progress);

  /// Periodically called on the main thread by MapFilesDownloader
  /// during the downloading process.
  void OnMapFileDownloadProgress(MapFilesDownloader::TProgress const & progress);

  bool RegisterDownloadedFiles(TCountryId const & countryId, MapOptions files);
  void OnMapDownloadFinished(TCountryId const & countryId, bool success, MapOptions files);

  /// Initiates downloading of the next file from the queue.
  void DownloadNextFile(QueuedCountry const & country);

public:
  /// \brief Storage will create its directories in Writable Directory
  /// (gotten with platform::WritableDir) by default.
  /// \param pathToCountriesFile is a name of countries.txt file.
  /// \param dataDir If |dataDir| is not empty Storage will create its directory in WritableDir/|dataDir|.
  /// \note if |dataDir| is not empty the instance of Storage can be used only for downloading map files
  /// but not for continue working with them.
  /// If |dataDir| is not empty the work flow is
  /// * create a instance of Storage with a special countries.txt and |dataDir|
  /// * download some maps to WritableDir/|dataDir|
  /// * destroy the instance of Storage and move the downloaded maps to proper place
  Storage(string const & pathToCountriesFile = COUNTRIES_FILE, string const & dataDir = string());
  /// \brief This constructor should be used for testing only.
  Storage(string const & referenceCountriesTxtJsonForTesting,
          unique_ptr<MapFilesDownloader> mapDownloaderForTesting);

  /// @name Interface with clients (Android/iOS).
  /// \brief It represents the interface which can be used by clients (Android/iOS).
  /// The term node means an mwm or a group of mwm like a big country.
  /// The term node id means a string id of mwm or a group of mwm. The sting contains
  /// a name of file with mwm of a name country(territory).
  //@{
  using TOnSearchResultCallback = function<void (TCountryId const &)>;
  using TOnStatusChangedCallback = function<void (TCountryId const &)>;

  /// \brief Information for "Update all mwms" button.
  struct UpdateInfo
  {
    size_t m_numberOfMwmFilesToUpdate;
    size_t m_totalUpdateSizeInBytes;
  };

  struct StatusCallback
  {
    /// \brief m_onStatusChanged is called by MapRepository when status of
    /// a node is changed. If this method is called for an mwm it'll be called for
    /// every its parent and grandparents.
    /// \param CountryId is id of mwm or an mwm group which status has been changed.
    TOnStatusChangedCallback m_onStatusChanged;
  };

  /// \brief Returns root country id of the county tree.
  TCountryId const GetRootId() const;
  /// \param childrenId is filled with children node ids by a parent. For example GetChildren(GetRootId())
  /// returns in param all countries ids. It's content of map downloader list by default.
  void GetChildren(TCountryId const & parent, TCountriesVec & childrenId) const;
  /// \brief Fills localChildren with children of parent.
  /// The result of the method is composed in a special way because of design requirements.
  /// If a direct child (of parent) contains two or more downloaded mwms the direct child id will be added to result.
  /// If a direct child (of parent) contains one downloaded mwm the mwm id will be added to result.
  /// If there's no downloaded mwms contained by a direct child the direct child id will not be added to result.
  /// \param parent is a parent acoording to countries.txt.
  /// \note. This method puts to localChildren only real maps which have been written in coutries.txt. It means
  /// the method does not put to localChildren neither custom maps generated by user
  /// nor World.mwm and WorldCoasts.mwm.
  void GetDownloadedChildren(TCountryId const & parent, TCountriesVec & localChildren) const;
  /// \brief Gets list of available countries.
  /// \param countryList is filled with a list of node id which an end user will see in
  /// a list of available maps. They are all available countries expect for fully downloaded
  /// countries. That means all mwm of the countries have been downloaded.
  void GetCountyListToDownload(TCountriesVec & countryList) const;
  /// \brief Calls |toDo| for each node for subtree with |root|.
  /// For example ForEachInSubtree(GetRootId()) calls |toDo| for every node including
  /// the result of GetRootId() call.
  void ForEachInSubtree(TCountryId const & root, TForEachFunction && toDo) const;

  /// \brief Returns current version for mwms which are available on the server.
  inline int64_t GetCurrentDataVersion() const { return m_currentVersion; }
  /// \brief Returns true if the node with countryId has been downloaded and false othewise.
  /// If countryId is a expandable returns true if all mwms which belongs to it have downloaded.
  /// Returns false if countryId is an unknown string.
  /// \note The method return false for custom maps generated by user
  /// and World.mwm and WorldCoasts.mwm.
  bool IsNodeDownloaded(TCountryId const & countryId) const;

  /// \brief Gets all the attributes for a node by its countryId.
  /// \param |nodeAttrs| is filled with attributes in this method.
  void GetNodeAttrs(TCountryId const & countryId, NodeAttrs & nodeAttrs) const;

  /// \brief Downloads one node (expandable or not) by countryId.
  /// If node is expandable downloads all children (grandchildren) by the node
  /// until they havn't been downloaded before. Update all downloaded mwm if it's necessary.
  bool DownloadNode(TCountryId const & countryId);
  /// \brief Delete one node (expandable or not).
  bool DeleteNode(TCountryId const & countryId);
  /// \brief Updates one node (expandable or not).
  /// \note If you want to update all the maps and this update is without changing
  /// borders or hierarchy just call UpdateNode(GetRootId()).
  /// \return false in case of error and true otherwise.
  bool UpdateNode(TCountryId const & countryId);
  /// \brief Cancels downloading a node if the downloading is in process.
  /// \return false in case of error and true otherwise.
  bool CancelDownloadNode(TCountryId const & countryId) { return true; }
  /// \brief Downloading process could be interupted because of bad internet connection.
  /// In that case user could want to recover it. This method is done for it.
  /// This method works with expandable and not expandable countryId.
  /// \return false in case of error and true otherwise.
  bool RetryDownloadNode(TCountryId const & countryId) { return true; }

  /// \brief Shows a node (expandable or not) on the map.
  /// \return false in case of error and true otherwise.
  bool ShowNode(TCountryId const & countryId);

  /// \brief Get information for mwm update button.
  /// \return true if updateInfo is filled correctly and false otherwise.
  bool GetUpdateInfo(TCountryId const & countryId, UpdateInfo & updateInfo) const { return true; }
  /// \brief Update all mwm in case of changing mwm hierarchy of mwm borders.
  /// This method:
  /// * removes all mwms
  /// * downloads mwms with the same coverage
  /// \note This method is used in very rare case.
  /// \return false in case of error and true otherwise.
  bool UpdateAllAndChangeHierarchy();

  /// \brief Subscribe on change status callback.
  /// \returns a unique index of added status callback structure.
  size_t SubscribeStatusCallback(StatusCallback const & statusCallbacks);
  /// \brief Unsubscribe from change status callback.
  /// \param index is a unique index of callback retruned by SubscribeStatusCallback.
  void UnsubscribeStatusCallback(size_t index);
  /// \brief Sets callback which will be called in case of a click on download map button on the map.
  void SetCallbackForClickOnDownloadMap(TDownloadFn & downloadFn) { m_downloadMapOnTheMap  = downloadFn; }
  /// \brief Calls |m_downloadMapOnTheMap| if one has been set.
  /// \param |countryId| is country id of a leaf. That means it's a file name.
  /// \note This method should be called for a click of download map button
  /// and for a click for retry downloading map button on the map.
  void DoClickOnDownloadMap(TCountryId const & countryId);
  //@}

  /// \returns real (not fake) local maps contained in countries.txt.
  /// So this method does not return custom user local maps and World and WorldCoosts country id.
  void GetLocalRealMaps(TCountriesVec & localMaps) const;

  void Init(TUpdate const & update);

  /// Do we have downloaded countries
  bool HaveDownloadedCountries() const;

  /// Prefetch MWMs before migrate
  unique_ptr<Storage> m_prefetchStorage;
  void PrefetchMigrateData();

  void SaveDownloadQueue();
  void RestoreDownloadQueue();


  /// Delete local maps and aggregate their Id if needed
  void DeleteAllLocalMaps(TCountriesVec * existedCountries = nullptr);

  /// Switch on new storage version, remove old mwm
  /// and add required mwm's into download queue.
  void Migrate(TCountriesVec const & existedCountries);

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

  Country const & CountryLeafByCountryId(TCountryId const & countryId) const;
  Country const & CountryByCountryId(TCountryId const & countryId) const;

  TCountryId FindCountryIdByFile(string const & name) const;
  /// @todo Temporary function to gel all associated indexes for the country file name.
  /// Will be removed in future after refactoring.
  TCountriesVec FindAllIndexesByFile(string const & name) const;
  void GetGroupAndCountry(TCountryId const & countryId, string & group, string & country) const;

  size_t CountriesCount(TCountryId const & countryId) const;
  string const & CountryName(TCountryId const & countryId) const;
  bool IsCoutryIdInCountryTree(TCountryId const & countryId) const;

  TLocalAndRemoteSize CountrySizeInBytes(TCountryId const & countryId, MapOptions opt) const;
  platform::CountryFile const & GetCountryFile(TCountryId const & countryId) const;
  TLocalFilePtr GetLatestLocalFile(platform::CountryFile const & countryFile) const;
  TLocalFilePtr GetLatestLocalFile(TCountryId const & countryId) const;

  /// Slow version, but checks if country is out of date
  Status CountryStatusEx(TCountryId const & countryId) const;

  /// Puts country denoted by countryId into the downloader's queue.
  /// During downloading process notifies observers about downloading
  /// progress and status changes.
  void DownloadCountry(TCountryId const & countryId, MapOptions opt);

  /// Removes country files (for all versions) from the device.
  /// Notifies observers about country status change.
  void DeleteCountry(TCountryId const & countryId, MapOptions opt);

  /// Removes country files of a particular version from the device.
  /// Notifies observers about country status change.
  void DeleteCustomCountryVersion(platform::LocalCountryFile const & localFile);

  /// \return True iff country denoted by countryId was successfully
  ///          deleted from the downloader's queue.
  bool DeleteFromDownloader(TCountryId const & countryId);
  bool IsDownloadInProgress() const;

  TCountryId GetCurrentDownloadingCountryId() const;

  void NotifyStatusChanged(TCountryId const & countryId);

  /// get download url by countryId & options(first search file name by countryId, then format url)
  string GetFileDownloadUrl(string const & baseUrl, TCountryId const & countryId, MapOptions file) const;
  /// get download url by base url & file name
  string GetFileDownloadUrl(string const & baseUrl, string const & fName) const;

  /// @param[out] res Populated with oudated countries.
  void GetOutdatedCountries(vector<Country const *> & countries) const;

  void SetDownloaderForTesting(unique_ptr<MapFilesDownloader> && downloader);
  void SetCurrentDataVersionForTesting(int64_t currentVersion);
  void SetDownloadingUrlsForTesting(vector<string> const & downloadingUrls)
  {
    m_downloadingUrlsForTesting = downloadingUrls;
  }

private:
  friend void UnitTest_StorageTest_DeleteCountrySingleMwm();
  friend void UnitTest_StorageTest_DeleteCountryTwoComponentsMwm();

  Status CountryStatusWithoutFailed(TCountryId const & countryId) const;
  Status CountryStatusFull(TCountryId const & countryId, Status const status) const;

  // Modifies file set of file to deletion - always adds (marks for
  // removal) a routing file when map file is marked for deletion.
  MapOptions NormalizeDeleteFileSet(MapOptions options) const;

  // Returns a pointer to a country in the downloader's queue.
  QueuedCountry * FindCountryInQueue(TCountryId const & countryId);

  // Returns a pointer to a country in the downloader's queue.
  QueuedCountry const * FindCountryInQueue(TCountryId const & countryId) const;

  // Returns true when country is in the downloader's queue.
  bool IsCountryInQueue(TCountryId const & countryId) const;

  // Returns true when country is first in the downloader's queue.
  bool IsCountryFirstInQueue(TCountryId const & countryId) const;

  // Returns local country files of a particular version, or wrapped
  // nullptr if there're no country files corresponding to the
  // version.
  TLocalFilePtr GetLocalFile(TCountryId const & countryId, int64_t version) const;

  // Tries to register disk files for a real (listed in countries.txt)
  // country. If map files of the same version were already
  // registered, does nothing.
  void RegisterCountryFiles(TLocalFilePtr localFile);

  // Registers disk files for a country. This method must be used only
  // for real (listed in countries.txt) countries.
  void RegisterCountryFiles(TCountryId const & countryId, string const & directory, int64_t version);

  // Registers disk files for a country. This method must be used only
  // for custom (made by user) map files.
  void RegisterFakeCountryFiles(platform::LocalCountryFile const & localFile);

  // Removes disk files for all versions of a country.
  void DeleteCountryFiles(TCountryId const & countryId, MapOptions opt);

  // Removes country files from downloader.
  bool DeleteCountryFilesFromDownloader(TCountryId const & countryId, MapOptions opt);

  // Returns download size of the currently downloading file for the
  // queued country.
  uint64_t GetDownloadSize(QueuedCountry const & queuedCountry) const;

  // Returns a path to a place on disk downloader can use for
  // downloaded files.
  string GetFileDownloadPath(TCountryId const & countryId, MapOptions file) const;

  void CountryStatusEx(TCountryId const & countryId, Status & status, MapOptions & options) const;
  /// Fast version, doesn't check if country is out of date
  Status CountryStatus(TCountryId const & countryId) const;
  /// Returns status for a node (group node or not)
  Status NodeStatus(TCountriesContainer const & node) const;
};

bool HasCountryId(TCountriesVec const & sorted, TCountryId const & countyId);
}  // storage
