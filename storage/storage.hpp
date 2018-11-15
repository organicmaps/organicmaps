#pragma once

#include "storage/country.hpp"
#include "storage/country_name_getter.hpp"
#include "storage/country_tree.hpp"
#include "storage/diff_scheme/diff_manager.hpp"
#include "storage/downloading_policy.hpp"
#include "storage/index.hpp"
#include "storage/map_files_downloader.hpp"
#include "storage/pinger.hpp"
#include "storage/queued_country.hpp"
#include "storage/storage_defines.hpp"

#include "platform/local_country_file.hpp"

#include "base/deferred_task.hpp"
#include "base/thread_checker.hpp"
#include "base/worker_thread.hpp"

#include "std/function.hpp"
#include "std/list.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

#include <boost/optional.hpp>

namespace storage
{
struct CountryIdAndName
{
  TCountryId m_id;
  string m_localName;

  bool operator==(CountryIdAndName const & other) const
  {
    return m_id == other.m_id && m_localName == other.m_localName;
  }
};

/// \brief Contains all properties for a node in the country tree.
/// It's applicable for expandable and not expandable node id.
struct NodeAttrs
{
  NodeAttrs() : m_mwmCounter(0), m_localMwmCounter(0), m_downloadingMwmCounter(0),
    m_mwmSize(0), m_localMwmSize(0), m_downloadingMwmSize(0),
    m_downloadingProgress(make_pair(0, 0)),
    m_status(NodeStatus::Undefined), m_error(NodeErrorCode::NoError), m_present(false) {}

  /// If the node is expandable (a big country) |m_mwmCounter| is number of mwm files (leaves)
  /// belonging to the node. If the node isn't expandable |m_mwmCounter| == 1.
  /// Note. For every expandable node |m_mwmCounter| >= 2.
  TMwmCounter m_mwmCounter;

  /// Number of mwms belonging to the node which have been downloaded.
  TMwmCounter m_localMwmCounter;

  /// Number of leaves of the node which have been downloaded
  /// plus which is in progress of downloading (zero or one)
  /// plus which are staying in queue.
  TMwmCounter m_downloadingMwmCounter;

  /// If it's not an expandable node, |m_mwmSize| is size of one mwm according to countries.txt.
  /// Otherwise |m_mwmSize| is the sum of all mwm file sizes which belong to the group
  /// according to countries.txt.
  TMwmSize m_mwmSize;

  /// If it's not an expandable node, |m_localMwmSize| is size of one downloaded mwm.
  /// Otherwise |m_localNodeSize| is the sum of all mwm file sizes which belong to the group and
  /// have been downloaded.
  TMwmSize m_localMwmSize;

  /// Size of leaves of the node which have been downloaded
  /// plus which is in progress of downloading (zero or one)
  /// plus which are staying in queue.
  /// \note The size of leaves is the size is written in countries.txt.
  TMwmSize m_downloadingMwmSize;

  /// The name of the node in a local language. That means the language dependent on
  /// a device locale.
  string m_nodeLocalName;

  /// The description of the node in a local language. That means the language dependent on
  /// a device locale.
  string m_nodeLocalDescription;

  /// Node id and local name of the parents of the node.
  /// For the root |m_parentInfo| is empty.
  /// Locale language is a language set by Storage::SetLocale().
  /// \note Most number of nodes have only one parent. But in case of a disputed territories
  /// an mwm could have two or even more parents. See Country description for details.
  vector<CountryIdAndName> m_parentInfo;

  /// Node id and local name of the first level parents (root children nodes)
  /// if the node has first level parent(s). Otherwise |m_topmostParentInfo| is empty.
  /// That means for the root and for the root children |m_topmostParentInfo| is empty.
  /// Locale language is a language set by Storage::SetLocale().
  /// \note Most number of nodes have only first level parent. But in case of a disputed territories
  /// an mwm could have two or even more parents. See Country description for details.
  vector<CountryIdAndName> m_topmostParentInfo;

  /// Progress of downloading for the node expandable or not. It reflects downloading progress in case of
  /// downloading and updating mwm.
  /// m_downloadingProgress.first is number of downloaded bytes.
  /// m_downloadingProgress.second is size of file(s) in bytes to download.
  /// So m_downloadingProgress.first <= m_downloadingProgress.second.
  MapFilesDownloader::TProgress m_downloadingProgress;

  /// Status of group and leaf node.
  /// For group nodes it's defined in the following way:
  /// If an mwm in a group has Downloading status the group has Downloading status
  /// Otherwise if an mwm in the group has InQueue status the group has InQueue status
  /// Otherwise if an mwm in the group has Error status the group has Error status
  /// Otherwise if an mwm in the group has OnDiskOutOfDate the group has OnDiskOutOfDate status
  /// Otherwise if all the mwms in the group have OnDisk status the group has OnDisk status
  /// Otherwise if all the mwms in the group have NotDownloaded status the group has NotDownloaded status
  /// Otherwise (that means a part of mwms in the group has OnDisk and the other part has NotDownloaded status)
  ///   the group has Mixed status
  NodeStatus m_status;
  /// Error code of leaf node. In case of group node |m_error| == NodeErrorCode::NoError.
  NodeErrorCode m_error;

  /// Indicates if leaf mwm is currently downloaded and connected to storage.
  /// Can be used to distinguish downloadable and updatable maps.
  /// m_present == false for group mwms.
  bool m_present;
};

/// \brief Statuses for a node in the country tree.
/// It's applicable for expandable and not expandable node id.
struct NodeStatuses
{
  NodeStatus m_status;
  NodeErrorCode m_error;
  bool m_groupNode;
};

/// This class is used for downloading, updating and deleting maps.
class Storage : public diffs::Manager::Observer
{
public:
  struct StatusCallback;
  using StartDownloadingCallback = function<void()>;
  using TUpdateCallback = function<void(storage::TCountryId const &, TLocalFilePtr const)>;
  using TDeleteCallback = function<bool(storage::TCountryId const &, TLocalFilePtr const)>;
  using TChangeCountryFunction = function<void(TCountryId const &)>;
  using TProgressFunction = function<void(TCountryId const &, MapFilesDownloader::TProgress const &)>;
  using TQueue = list<QueuedCountry>;

private:
  /// We support only one simultaneous request at the moment
  unique_ptr<MapFilesDownloader> m_downloader;

  /// Stores timestamp for update checks
  int64_t m_currentVersion;

  TCountryTree m_countries;

  /// @todo. It appeared that our application uses m_queue from
  /// different threads without any synchronization. To reproduce it
  /// just download a map "from the map" on Android. (CountryStatus is
  /// called from a different thread.)  It's necessary to check if we
  /// can call all the methods from a single thread using
  /// RunOnUIThread.  If not, at least use a syncronization object.
  TQueue m_queue;

  // Keep downloading queue between application restarts.
  bool m_keepDownloadingQueue = true;

  /// Set of mwm files which have been downloaded recently.
  /// When a mwm file is downloaded it's moved from |m_queue| to |m_justDownloaded|.
  /// When a new mwm file is added to |m_queue| |m_justDownloaded| is cleared.
  /// Note. This set is necessary for implementation of downloading progress of
  /// mwm group.
  TCountriesSet m_justDownloaded;

  /// stores countries whose download has failed recently
  TCountriesSet m_failedCountries;

  map<TCountryId, list<TLocalFilePtr>> m_localFiles;

  // Our World.mwm and WorldCoasts.mwm are fake countries, together with any custom mwm in data
  // folder.
  map<platform::CountryFile, TLocalFilePtr> m_localFilesForFakeCountries;

  /// used to correctly calculate total country download progress with more than 1 file
  /// <current, total>
  MapFilesDownloader::TProgress m_countryProgress;

  DownloadingPolicy m_defaultDownloadingPolicy;
  DownloadingPolicy * m_downloadingPolicy = &m_defaultDownloadingPolicy;

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
  // country are successfully downloaded.
  TUpdateCallback m_didDownload;

  // This function is called each time all files for a
  // country are deleted.
  TDeleteCallback m_willDelete;

  // If |m_dataDir| is not empty Storage will create version directories and download maps in
  // platform::WritableDir/|m_dataDir|/. Not empty |m_dataDir| can be used only for
  // downloading maps to a special place but not for continue working with them from this place.
  string m_dataDir;

  // A list of urls for downloading maps. It's necessary for storage integration tests.
  // For example {"http://new-search.mapswithme.com/"}.
  vector<string> m_downloadingUrlsForTesting;

  bool m_integrityValidationEnabled = true;

  // |m_downloadMapOnTheMap| is called when an end user clicks on download map or retry button
  // on the map.
  TDownloadFn m_downloadMapOnTheMap;

  CountryNameGetter m_countryNameGetter;

  unique_ptr<Storage> m_prefetchStorage;

  // |m_affiliations| is a mapping from countryId to the list of names of
  // geographical objects (such as countries) that encompass this countryId.
  // Note. Affiliations is inherited from ancestors of the countryId in country tree.
  // |m_affiliations| is filled during Storage initialization or during migration process.
  // It is filled with data of countries.txt (field "affiliations").
  // Once filled |m_affiliations| is not changed.
  // Note. |m_affiliations| is empty in case of countries_obsolete.txt.
  TMappingAffiliations m_affiliations;

  TMwmSize m_maxMwmSizeBytes;

  ThreadChecker m_threadChecker;

  diffs::Manager m_diffManager;
  vector<platform::LocalCountryFile> m_notAppliedDiffs;

  bool m_needToStartDeferredDownloading = false;
  boost::optional<vector<string>> m_sessionServerList;

  StartDownloadingCallback m_startDownloadingCallback;

  void DownloadNextCountryFromQueue();

  void LoadCountriesFile(string const & pathToCountriesFile, string const & dataDir,
                         TMappingOldMwm * mapping = nullptr);

  void ReportProgress(TCountryId const & countryId, MapFilesDownloader::TProgress const & p);
  void ReportProgressForHierarchy(TCountryId const & countryId,
                                  MapFilesDownloader::TProgress const & leafProgress);

  void DoDownload();
  void SetDeferDownloading();
  void DoDeferredDownloadIfNeeded();

  /// Called on the main thread by MapFilesDownloader when
  /// downloading of a map file succeeds/fails.
  void OnMapFileDownloadFinished(downloader::HttpRequest::Status status,
                                 MapFilesDownloader::TProgress const & progress);

  /// Periodically called on the main thread by MapFilesDownloader
  /// during the downloading process.
  void OnMapFileDownloadProgress(MapFilesDownloader::TProgress const & progress);

  void RegisterDownloadedFiles(TCountryId const & countryId, MapOptions files);

  void OnMapDownloadFinished(TCountryId const & countryId,
                             downloader::HttpRequest::Status status,
                             MapOptions files);

  /// Initiates downloading of the next file from the queue.
  void DownloadNextFile(QueuedCountry const & country);

public:
  ThreadChecker const & GetThreadChecker() const {return m_threadChecker;}

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

  void Init(TUpdateCallback const & didDownload, TDeleteCallback const & willDelete);

  inline void SetDownloadingPolicy(DownloadingPolicy * policy) { m_downloadingPolicy = policy; }

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
    UpdateInfo() : m_numberOfMwmFilesToUpdate(0), m_totalUpdateSizeInBytes(0), m_sizeDifference(0) {}

    TMwmCounter m_numberOfMwmFilesToUpdate;
    TMwmSize m_totalUpdateSizeInBytes;
    // Difference size in bytes between before update and after update.
    int64_t m_sizeDifference;
  };

  struct StatusCallback
  {
    /// \brief m_onStatusChanged is called by MapRepository when status of
    /// a node is changed. If this method is called for an mwm it'll be called for
    /// every its parent and grandparents.
    /// \param CountryId is id of mwm or an mwm group which status has been changed.
    TOnStatusChangedCallback m_onStatusChanged;
  };

  bool CheckFailedCountries(TCountriesVec const & countries) const;

  /// \brief Returns root country id of the country tree.
  TCountryId const GetRootId() const;

  /// \param childIds is filled with children node ids by a parent. For example GetChildren(GetRootId())
  /// returns in param all countries ids. It's content of map downloader list by default.
  void GetChildren(TCountryId const & parent, TCountriesVec & childIds) const;

  /// \brief Fills |downloadedChildren| and |availChildren| with children of parent.
  /// If a direct child of |parent| contains at least one downloaded mwm
  /// the mwm id of the child will be added to |downloadedChildren|.
  /// If not, the mwm id the child will not be added to |availChildren|.
  /// \param parent is a parent acoording to countries.txt or cournties_migrate.txt.
  /// \param downloadedChildren children partly or fully downloaded.
  /// \param availChildren fully available children. None of its files have been downloaded.
  /// \param keepAvailableChildren keeps all children in |availChildren| otherwise downloaded
  ///        children will be removed from |availChildren|.
  /// \note. This method puts to |downloadedChildren| and |availChildren| only real maps (and its ancestors)
  /// which have been written in coutries.txt or cournties_migrate.txt.
  /// It means the method does not put to its params neither custom maps generated by user
  /// nor World.mwm and WorldCoasts.mwm.
  void GetChildrenInGroups(TCountryId const & parent,
                           TCountriesVec & downloadedChildren, TCountriesVec & availChildren,
                           bool keepAvailableChildren = false) const;
  /// \brief Fills |queuedChildren| with children of |parent| if they (or thier childen) are in |m_queue|.
  /// \note For group node children if one of child's ancestor has status
  /// NodeStatus::Downloading or NodeStatus::InQueue the child is considered as a queued child
  /// and will be added to |queuedChildren|.
  void GetQueuedChildren(TCountryId const & parent, TCountriesVec & queuedChildren) const;

  /// \brief Fills |path| with list of TCountryId corresponding with path to the root of hierachy.
  /// \param groupNode is start of path, can't be a leaf node.
  /// \param path is resulting array of TCountryId.
  void GetGroupNodePathToRoot(TCountryId const & groupNode, TCountriesVec & path) const;

  /// \brief Fills |nodes| with CountryIds of topmost nodes for this |countryId|.
  /// \param level is distance from top level except root.
  /// For disputed territories all possible owners will be added.
  /// Puts |countryId| to |nodes| when |level| is greater than the level of |countyId|. 
  void GetTopmostNodesFor(TCountryId const & countryId, TCountriesVec & nodes, size_t level = 0) const;

  /// \brief Returns parent id for node if node has single parent. Otherwise (if node is disputed
  /// territory and has multiple parents or does not exist) returns empty TCountryId
  TCountryId const GetParentIdFor(TCountryId const & countryId) const;

  /// \brief Returns current version for mwms which are used by storage.
  inline int64_t GetCurrentDataVersion() const { return m_currentVersion; }

  /// \brief Returns true if the node with countryId has been downloaded and false othewise.
  /// If countryId is expandable returns true if all mwms which belongs to it have downloaded.
  /// Returns false if countryId is an unknown string.
  /// \note The method return false for custom maps generated by user
  /// and World.mwm and WorldCoasts.mwm.
  bool IsNodeDownloaded(TCountryId const & countryId) const;

  /// \brief Returns true if the last version of countryId has been downloaded.
  bool HasLatestVersion(TCountryId const & countryId) const;

  /// \brief Gets all the attributes for a node by its |countryId|.
  /// \param |nodeAttrs| is filled with attributes in this method.
  void GetNodeAttrs(TCountryId const & countryId, NodeAttrs & nodeAttrs) const;
  /// \brief Gets a short list of node attributes by its |countriId|.
  /// \note This method works quicklier than GetNodeAttrs().
  void GetNodeStatuses(TCountryId const & countryId, NodeStatuses & nodeStatuses) const;

  string GetNodeLocalName(TCountryId const & countryId) const { return m_countryNameGetter(countryId); }

  /// \brief Downloads/update one node (expandable or not) by countryId.
  /// If node is expandable downloads/update all children (grandchildren) by the node
  /// until they haven't been downloaded before.
  void DownloadNode(TCountryId const & countryId, bool isUpdate = false);

  /// \brief Delete node with all children (expandable or not).
  void DeleteNode(TCountryId const & countryId);

  /// \brief Updates one node. It works for leaf and group mwms.
  /// \note If you want to update all the maps and this update is without changing
  /// borders or hierarchy just call UpdateNode(GetRootId()).
  void UpdateNode(TCountryId const & countryId);

  /// \brief If the downloading a new node is in process cancels downloading the node and deletes
  /// the downloaded part of the map. If the map is in queue, remove the map from the queue.
  /// If the downloading a updating map is in process cancels the downloading,
  /// deletes the downloaded part of the map and leaves as is the old map (before the update)
  /// had been downloaded. It works for leaf and for group mwms.
  void CancelDownloadNode(TCountryId const & countryId);

  /// \brief Downloading process could be interupted because of bad internet connection
  /// and some other reason.
  /// In that case user could want to recover it. This method is done for it.
  /// This method works with leaf and group mwm.
  /// In case of a group mwm this method retries downloading all mwm in m_failedCountries list
  /// which in the subtree with root |countryId|.
  /// It means the call RetryDownloadNode(GetRootId()) retries all the failed mwms.
  void RetryDownloadNode(TCountryId const & countryId);

  /// \brief Get information for mwm update button.
  /// \return true if updateInfo is filled correctly and false otherwise.
  bool GetUpdateInfo(TCountryId const & countryId, UpdateInfo & updateInfo) const;

  TMappingAffiliations const & GetAffiliations() const { return m_affiliations; }

  /// \brief Calls |toDo| for each node for subtree with |root|.
  /// For example ForEachInSubtree(GetRootId()) calls |toDo| for every node including
  /// the result of GetRootId() call.
  template <class ToDo>
  void ForEachInSubtree(TCountryId const & root, ToDo && toDo) const;
  template <class ToDo>
  void ForEachAncestorExceptForTheRoot(TCountryId const & childId, ToDo && toDo) const;
  template <class ToDo>
  void ForEachCountryFile(ToDo && toDo) const;

  /// \brief Sets callback which will be called in case of a click on download map button on the map.
  void SetCallbackForClickOnDownloadMap(TDownloadFn & downloadFn);

  /// \brief Calls |m_downloadMapOnTheMap| if one has been set.
  /// \param |countryId| is country id of a leaf. That means it's a file name.
  /// \note This method should be called for a click of download map button
  /// and for a click for retry downloading map button on the map.
  void DoClickOnDownloadMap(TCountryId const & countryId);
  //@}

  /// \returns real (not fake) local maps contained in countries.txt.
  /// So this method does not return custom user local maps and World and WorldCoasts country id.
  void GetLocalRealMaps(TCountriesVec & localMaps) const;

  /// Do we have downloaded countries
  bool HaveDownloadedCountries() const;

  /// Delete local maps and aggregate their Id if needed
  void DeleteAllLocalMaps(TCountriesVec * existedCountries = nullptr);

  /// Prefetch MWMs before migrate
  Storage * GetPrefetchStorage();
  void PrefetchMigrateData();

  /// Switch on new storage version, remove old mwm
  /// and add required mwm's into download queue.
  void Migrate(TCountriesVec const & existedCountries);

  // Clears local files registry and downloader's queue.
  void Clear();

  // Finds and registers all map files in maps directory. In the case
  // of several versions of the same map keeps only the latest one, others
  // are deleted from disk.
  // *NOTE* storage will forget all already known local maps.
  void RegisterAllLocalMaps(bool enableDiffs);

  // Returns list of all local maps, including fake countries (World*.mwm).
  void GetLocalMaps(vector<TLocalFilePtr> & maps) const;

  // Returns number of downloaded maps (files), excluding fake countries (World*.mwm).
  size_t GetDownloadedFilesCount() const;

  /// Guarantees that change and progress are called in the main thread context.
  /// @return unique identifier that should be used with Unsubscribe function
  int Subscribe(TChangeCountryFunction const & change, TProgressFunction const & progress);
  void Unsubscribe(int slotId);

  /// Returns information about selected counties downloading progress.
  /// |countries| - watched CountryId, ONLY leaf expected.
  MapFilesDownloader::TProgress GetOverallProgress(TCountriesVec const &countries) const;

  Country const & CountryLeafByCountryId(TCountryId const & countryId) const;
  Country const & CountryByCountryId(TCountryId const & countryId) const;

  TCountryId FindCountryIdByFile(string const & name) const;

  // These two functions check whether |countryId| is a leaf
  // or an inner node of the country tree.
  bool IsLeaf(TCountryId const & countryId) const;
  bool IsInnerNode(TCountryId const & countryId) const;

  TLocalAndRemoteSize CountrySizeInBytes(TCountryId const & countryId, MapOptions opt) const;
  TMwmSize GetRemoteSize(platform::CountryFile const & file, MapOptions opt, int64_t version) const;
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

  /// \brief Deletes countryId from the downloader's queue.
  void DeleteFromDownloader(TCountryId const & countryId);
  bool IsDownloadInProgress() const;

  TCountryId GetCurrentDownloadingCountryId() const;
  void EnableKeepDownloadingQueue(bool enable) {m_keepDownloadingQueue = enable;}
  /// get download url by base url & queued country
  string GetFileDownloadUrl(string const & baseUrl, QueuedCountry const & queuedCountry) const;
  string GetFileDownloadUrl(string const & baseUrl, string const & fileName) const;

  /// @param[out] res Populated with oudated countries.
  void GetOutdatedCountries(vector<Country const *> & countries) const;

  /// Sets and gets locale, which is used to get localized counries names
  void SetLocale(string const & locale);
  string GetLocale() const;
  
  TMwmSize GetMaxMwmSizeBytes() const { return m_maxMwmSizeBytes; }

  // for testing:
  void SetEnabledIntegrityValidationForTesting(bool enabled);
  void SetDownloaderForTesting(unique_ptr<MapFilesDownloader> && downloader);
  void SetCurrentDataVersionForTesting(int64_t currentVersion);
  void SetDownloadingUrlsForTesting(vector<string> const & downloadingUrls);
  void SetLocaleForTesting(string const & jsonBuffer, string const & locale);

  /// Returns true if the diff scheme is available and all local outdated maps can be updated via
  /// diffs.
  bool IsPossibleToAutoupdate() const;

  void SetStartDownloadingCallback(StartDownloadingCallback const & cb);

private:
  friend struct UnitClass_StorageTest_DeleteCountry;
  friend struct UnitClass_TwoComponentStorageTest_DeleteCountry;

  void SaveDownloadQueue();
  void RestoreDownloadQueue();

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

  // Returns true if we started the diff applying procedure for an mwm with countryId.
  bool IsDiffApplyingInProgressToCountry(TCountryId const & countryId) const;

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
  void DeleteCountryFiles(TCountryId const & countryId, MapOptions opt, bool deferredDelete);

  // Removes country files from downloader.
  bool DeleteCountryFilesFromDownloader(TCountryId const & countryId);

  // Returns download size of the currently downloading file for the
  // queued country.
  uint64_t GetDownloadSize(QueuedCountry const & queuedCountry) const;

  // Returns a path to a place on disk downloader can use for
  // downloaded files.
  string GetFileDownloadPath(TCountryId const & countryId, MapOptions file) const;

  void CountryStatusEx(TCountryId const & countryId, Status & status, MapOptions & options) const;

  /// Fast version, doesn't check if country is out of date
  Status CountryStatus(TCountryId const & countryId) const;

  /// Returns status for a node (group node or not).
  StatusAndError GetNodeStatus(TCountryTreeNode const & node) const;
  /// Returns status for a node (group node or not).
  /// Fills |disputedTeritories| with all disputed teritories in subtree with the root == |node|.
  StatusAndError GetNodeStatusInfo(TCountryTreeNode const & node,
                                   vector<pair<TCountryId, NodeStatus>> & disputedTeritories,
                                   bool isDisputedTerritoriesCounted) const;

  void NotifyStatusChanged(TCountryId const & countryId);
  void NotifyStatusChangedForHierarchy(TCountryId const & countryId);

  /// @todo Temporary function to gel all associated indexes for the country file name.
  /// Will be removed in future after refactoring.
  TCountriesVec FindAllIndexesByFile(TCountryId const & name) const;

  /// Calculates progress of downloading for expandable nodes in country tree.
  /// |descendants| All descendants of the parent node.
  /// |downloadingMwm| Downloading leaf node country id if any. If not, downloadingMwm == kInvalidCountryId.
  /// |downloadingMwm| Must be only leaf.
  /// If downloadingMwm != kInvalidCountryId |downloadingMwmProgress| is a progress of downloading
  /// the leaf node in bytes. |downloadingMwmProgress.first| == number of downloaded bytes.
  /// |downloadingMwmProgress.second| == number of bytes in downloading files.
  /// |mwmsInQueue| hash table made from |m_queue|.
  MapFilesDownloader::TProgress CalculateProgress(TCountryId const & downloadingMwm,
                                                  TCountriesVec const & descendants,
                                                  MapFilesDownloader::TProgress const & downloadingMwmProgress,
                                                  TCountriesSet const & mwmsInQueue) const;

  void PushToJustDownloaded(TQueue::iterator justDownloadedItem);
  void PopFromQueue(TQueue::iterator it);
  template <class ToDo>
  void ForEachAncestorExceptForTheRoot(vector<TCountryTreeNode const *> const & nodes, ToDo && toDo) const;
  /// Returns true if |node.Value().Name()| is a disputed territory and false otherwise.
  bool IsDisputed(TCountryTreeNode const & node) const;

  void CalMaxMwmSizeBytes();
  
  void OnDownloadFailed(TCountryId const & countryId);

  void LoadDiffScheme();
  void ApplyDiff(TCountryId const & countryId, function<void(bool isSuccess)> const & fn);

  // Should be called once on startup, downloading process should be suspended until this method
  // was not called. Do not call this method manually.
  void OnDiffStatusReceived(diffs::Status const status) override;

  void LoadServerListForSession();
  void LoadServerListForTesting();
  void PingServerList(vector<string> const & urls);
};

void GetQueuedCountries(Storage::TQueue const & queue, TCountriesSet & resultCountries);

template <class ToDo>
void Storage::ForEachInSubtree(TCountryId const & root, ToDo && toDo) const
{
  TCountryTreeNode const * const rootNode = m_countries.FindFirst(root);
  if (rootNode == nullptr)
  {
    ASSERT(false, ("TCountryId =", root, "not found in m_countries."));
    return;
  }
  rootNode->ForEachInSubtree([&toDo](TCountryTreeNode const & container)
                             {
                               Country const & value = container.Value();
                               toDo(value.Name(),
                                    value.GetSubtreeMwmCounter() != 1 /* groupNode. */);
                             });
}

/// Calls functor |toDo| with signature
/// void(const TCountryId const & parentId, TCountriesVec const & descendantCountryId)
/// for each ancestor except for the main root of the tree in order from the leaf to the root.
/// Note. In case of disputable territories several nodes with the same name may be
/// present in the country tree. In that case ForEachAncestorExceptForTheRoot calls
/// |toDo| for parents of each way to the root in the country tree. In case of diamond
/// trees toDo is called for common part of ways to the root only once.
template <class ToDo>
void Storage::ForEachAncestorExceptForTheRoot(TCountryId const & countryId, ToDo && toDo) const
{
  vector<TCountryTreeNode const *> nodes;
  m_countries.Find(countryId, nodes);
  if (nodes.empty())
  {
    ASSERT(false, ("TCountryId =", countryId, "not found in m_countries."));
    return;
  }

  ForEachAncestorExceptForTheRoot(nodes, forward<ToDo>(toDo));
}

template <class ToDo>
void Storage::ForEachAncestorExceptForTheRoot(vector<TCountryTreeNode const *> const & nodes, ToDo && toDo) const
{
  set<TCountryTreeNode const *> visitedAncestors;
  // In most cases nodes.size() == 1. In case of disputable territories nodes.size()
  // may be more than one. It means |childId| is present in the country tree more than once.
  for (auto const & node : nodes)
  {
    node->ForEachAncestorExceptForTheRoot(
        [&](TCountryTreeNode const & container)
        {
          TCountryId const ancestorId = container.Value().Name();
          if (visitedAncestors.find(&container) != visitedAncestors.end())
            return;  // The node was visited before because countryId is present in the tree more
                     // than once.
          visitedAncestors.insert(&container);
          toDo(ancestorId, container);
        });
  }
}

template <class ToDo>
void Storage::ForEachCountryFile(ToDo && toDo) const
{
  m_countries.GetRoot().ForEachInSubtree([&](TCountryTree::Node const & node) {
    if (node.ChildrenCount() == 0)
      toDo(node.Value().GetFile());
  });
}
}  // storage
