#pragma once

#include "storage/country.hpp"
#include "storage/country_name_getter.hpp"
#include "storage/country_tree.hpp"
#include "storage/diff_scheme/diffs_data_source.hpp"
#include "storage/downloading_policy.hpp"
#include "storage/map_files_downloader.hpp"
#include "storage/queued_country.hpp"
#include "storage/storage_defines.hpp"

#include "indexer/terrain/twm_grid.hpp"

#include "platform/downloader_defines.hpp"
#include "platform/local_country_file.hpp"

#include "base/cancellable.hpp"
#include "base/thread_checker.hpp"

#include "defines.hpp"

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace storage_tests
{
struct UnitClass_StorageTest_DeleteCountry;
}  // namespace storage_tests

namespace storage
{
struct CountryIdAndName
{
  CountryId m_id;
  std::string m_localName;

  bool operator==(CountryIdAndName const & other) const
  {
    return m_id == other.m_id && m_localName == other.m_localName;
  }
};

/// \brief Contains all properties for a node in the country tree.
/// It's applicable for expandable and not expandable node id.
struct NodeAttrs
{
  NodeAttrs()
    : m_mwmCounter(0)
    , m_localMwmCounter(0)
    , m_downloadingMwmCounter(0)
    , m_mwmSize(0)
    , m_localMwmSize(0)
    , m_downloadingMwmSize(0)
    , m_status(NodeStatus::Undefined)
    , m_error(NodeErrorCode::NoError)
    , m_present(false)
  {}

  /// If the node is expandable (a big country) |m_mwmCounter| is number of mwm files (leaves)
  /// belonging to the node. If the node isn't expandable |m_mwmCounter| == 1.
  /// Note. For every expandable node |m_mwmCounter| >= 2.
  MwmCounter m_mwmCounter;

  /// Number of mwms belonging to the node which have been downloaded.
  MwmCounter m_localMwmCounter;

  /// Number of leaves of the node which have been downloaded
  /// plus which is in progress of downloading (zero or one)
  /// plus which are staying in queue.
  MwmCounter m_downloadingMwmCounter;

  /// If it's not an expandable node, |m_mwmSize| is size of one mwm according to countries.json.
  /// Otherwise |m_mwmSize| is the sum of all mwm file sizes which belong to the group
  /// according to countries.json.
  MwmSize m_mwmSize;

  /// If it's not an expandable node, |m_localMwmSize| is size of one downloaded mwm.
  /// Otherwise |m_localNodeSize| is the sum of all mwm file sizes which belong to the group and
  /// have been downloaded.
  MwmSize m_localMwmSize;

  /// Size of leaves of the node which have been downloaded
  /// plus which is in progress of downloading (zero or one)
  /// plus which are staying in queue.
  /// \note The size of leaves is the size is written in countries.json.
  MwmSize m_downloadingMwmSize;

  /// The name of the node in a local language. That means the language dependent on
  /// a device locale.
  std::string m_nodeLocalName;

  /// The description of the node in a local language. That means the language dependent on
  /// a device locale.
  std::string m_nodeLocalDescription;

  /// Node id and local name of the parents of the node.
  /// For the root |m_parentInfo| is empty.
  /// Locale language is a language set by Storage::SetLocale().
  /// \note Most number of nodes have only one parent. But in case of a disputed territories
  /// an mwm could have two or even more parents. See Country description for details.
  std::vector<CountryIdAndName> m_parentInfo;

  /// Node id and local name of the first level parents (root children nodes)
  /// if the node has first level parent(s). Otherwise |m_topmostParentInfo| is empty.
  /// That means for the root and for the root children |m_topmostParentInfo| is empty.
  /// Locale language is a language set by Storage::SetLocale().
  /// \note Most number of nodes have only first level parent. But in case of a disputed territories
  /// an mwm could have two or even more parents. See Country description for details.
  std::vector<CountryIdAndName> m_topmostParentInfo;

  /// Progress of downloading for the node expandable or not. It reflects downloading progress in case of
  /// downloading and updating mwm.
  /// m_downloadingProgress.m_bytesDownloaded is number of downloaded bytes.
  /// m_downloadingProgress.m_bytesTotal is size of file(s) in bytes to download.
  /// So m_downloadingProgress.m_bytesDownloaded <= m_downloadingProgress.m_bytesTotal.
  downloader::Progress m_downloadingProgress;

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

// This class is used for downloading, updating and deleting maps.
// Storage manages a queue of mwms to be downloaded.
// Every operation with this queue must be executed
// on the storage thread. In the current implementation, the storage
// thread coincides with the main (UI) thread.
// Downloading of only one mwm at a time is supported, so while the
// mwm at the top of the queue is being downloaded (or updated by
// applying a diff file) all other mwms have to wait.
class Storage final : public QueuedCountry::Subscriber
{
public:
  using StartDownloadingCallback = std::function<void()>;
  using UpdateCallback = std::function<void(storage::CountryId const &, LocalFilePtr const)>;
  using DeleteCallback = std::function<bool(storage::CountryId const &, LocalFilePtr const)>;
  // The landed block file and its grid rect; the receiver registers the block and
  // replaces the outdated coverage (TerrainProvider::OnBlockDownloaded).
  using TerrainDownloadedFn = std::function<void(std::string const & path, m2::RectD const &)>;
  // True when terrain older than the version covers the mercator rect
  // (TerrainProvider::HasOlderTerrain): the OnDiskOutOfDate status source.
  using TerrainHasOlderFn = std::function<bool(m2::RectD const &, int64_t version)>;
  // Deletes the terrain blocks intersecting the rects (TerrainProvider::DeleteBlocks).
  using TerrainDeleteFn = std::function<void(std::vector<m2::RectD> const &)>;
  using ChangeCountryFunction = std::function<void(CountryId const &)>;
  using ProgressFunction = std::function<void(CountryId const &, downloader::Progress const &)>;
  using DownloadingCountries = std::unordered_map<CountryId, downloader::Progress>;

private:
  /// We support only one simultaneous request at the moment
  std::unique_ptr<MapFilesDownloader> m_downloader;

  /// Stores timestamp for update checks
  int64_t m_currentVersion = 0;

  CountryTree m_countries;

  /// Set of mwm files which have been downloaded recently.
  /// When a mwm file is downloaded it's added to |m_justDownloaded|.
  /// Note. This set is necessary for implementation of downloading progress of
  /// mwm group.
  CountriesSet m_justDownloaded;

  /// stores countries whose download has failed recently
  CountriesSet m_failedCountries;

  /// Usually the list value has only 1 entry.
  /// 2 entries are possible in a moment of updating a map (old and new are present).
  std::map<CountryId, std::list<LocalFilePtr>> m_localFiles;

  // World and WorldCoasts are fake countries, together with any custom mwm in data folder.
  // Together with "old" (outdated) countries, that were splitted with the new regions set.
  std::map<platform::CountryFile, LocalFilePtr> m_localFilesForFakeCountries;

  // Since the diffs applying runs on a different thread, the result
  // of diff application may return "Ok" when in fact the diff was
  // cancelled. However, the storage thread knows for sure whether
  // request was to apply or to cancel the diff, and this knowledge
  // is represented by |m_diffsBeingApplied|.
  std::unordered_map<CountryId, std::unique_ptr<base::Cancellable>> m_diffsBeingApplied;

  std::vector<platform::LocalCountryFile> m_notAppliedDiffs;

  diffs::DiffsSourcePtr m_diffsDataSource = std::make_shared<diffs::DiffsDataSource>();

  DownloadingPolicy m_defaultDownloadingPolicy;
  DownloadingPolicy * m_downloadingPolicy = &m_defaultDownloadingPolicy;

  /// @name Communicate with GUI
  //@{

  int m_currentSlotId = 0;

  struct CountryObservers
  {
    ChangeCountryFunction m_changeCountryFn;
    ProgressFunction m_progressFn;
    int m_slotId;
  };

  std::list<CountryObservers> m_observers;
  //@}

  // This function is called each time all files requested for a
  // country are successfully downloaded.
  UpdateCallback m_didDownload;

public:
  // The terrain blocks grid (data/twm_grid.json; empty when the bundle has none) with
  // the countries.json-style size and integrity hash per block, and the
  // Framework-injected terrain hooks.
  // Public with ParseTwmGridJson for the tests only.
  struct TerrainBlock
  {
    terrain::GridBlock m_block;
    // Precomputed at the parse time: the attrs fusion runs per downloader row on the
    // GUI thread, no per-call name formatting or lat/lon conversions there.
    std::string m_name;  // The block name without the extension, the download id.
    m2::RectD m_rect;    // m_block.GetRectMercator().
    uint64_t m_size = 0;
    std::string m_hash;
    // The version of the last snapshot that changed the block bytes: names the download
    // URL and the client folder terrain/<version>/, so a grid update touches only the
    // re-generated blocks and the unchanged files simply stay current in their folders.
    int64_t m_version = 0;
    // The current-version file presence: seeded by the provider scan (OnTerrainScanned)
    // and maintained incrementally by the downloads and the deletes.
    bool m_onDisk = false;
  };

  // Throws RootException on any inconsistency leaving the out params untouched.
  static void ParseTwmGridJson(std::string const & jsonBuffer, int64_t & version, std::vector<TerrainBlock> & blocks,
                               std::map<CountryId, std::vector<uint32_t>> & coverage);

  // The on-disk truth from TerrainProvider::Rescan (the async RegisterAllMaps flow):
  // every registered file - a file the provider condemned or deleted does not count.
  // Until this lands the terrain is unknown: the attrs carry no terrain component and
  // nothing terrain downloads or resumes.
  void OnTerrainScanned(std::vector<terrain::TwmFile> const & scanned);

private:
  std::vector<TerrainBlock> m_twmGrid;
  // Region id -> the m_twmGrid indices of the blocks intersecting the region polygon
  // (twm_grid.json "mwms"): the download/status/delete unit is the region, no client
  // geometry involved. Group nodes resolve as the union of their leafs.
  std::map<CountryId, std::vector<uint32_t>> m_terrainCoverage;
  // See OnTerrainScanned and RestoreTerrain: the resume runs after both have landed.
  bool m_terrainScanned = false;
  bool m_queueRestored = false;
  std::vector<terrain::TwmFile> m_scannedTerrain;
  // Calls fn(leafId, blockIndices) for the countryId coverage leaf or for every
  // coverage leaf of the subtree of a group.
  template <class Fn>
  void ForEachCoverageLeaf(CountryId const & countryId, Fn && fn) const;
  // Calls fn(leafId, blockIndex) for every block the countryId subtree still has to
  // download: the single definition of "the terrain part of an update" shared by
  // GetUpdateInfo (the size) and UpdateNode (the downloads), so the two cannot drift.
  template <class Fn>
  void ForEachTerrainBlockToDownload(CountryId const & countryId, Fn && fn) const;
  // The partial downloader artifacts of the block: the resume record of an interrupted
  // download (see RestoreTerrain), deleted by the cancel so it stays cancelled.
  void DeleteTerrainArtifacts(TerrainBlock const & block) const;
  // Sweeps the on-disk blocks and the downloader artifacts nobody wants and resumes the
  // regions whose interrupted downloads left artifacts behind - the disk is the only
  // record, there is no settings snapshot of the terrain intent (the maps are the intent).
  void RestoreTerrain();
  TerrainDownloadedFn m_terrainDownloadedFn;
  TerrainHasOlderFn m_terrainHasOlderFn;
  TerrainDeleteFn m_terrainDeleteFn;

  // The m_twmGrid indices covering the countryId: the coverage list of a leaf or the
  // sorted deduplicated union over the subtree leafs of a group.
  std::vector<uint32_t> GetCoveringBlocks(CountryId const & countryId) const;
  // The m_twmGrid indices the downloaded and the queued regions cover, minus the
  // excluded ones: the protection set of the ref-counted delete and of the orphan sweep.
  std::set<uint32_t> GetWantedTerrainBlocks(CountriesSet const & excluded) const;

  // The terrain contribution to the region attrs: the full coverage bytes join the
  // region size, the in-flight blocks its download progress and the missing ones flip
  // a map-complete region to OnDiskOutOfDate.
  struct TerrainFusion
  {
    uint64_t m_coverageBytes = 0;       // The whole subtree coverage, deduplicated.
    uint64_t m_onDiskBytes = 0;         // The current-version files present.
    uint64_t m_localBytes = 0;          // Registered files of any version serving the coverage.
    uint64_t m_obsoleteLocalBytes = 0;  // Local bytes not represented by m_onDiskBytes.
    uint64_t m_missingBytes = 0;        // Not on disk, not in flight; downloaded leafs only.
    uint64_t m_inFlightTotal = 0;       // Queued or downloading.
    uint64_t m_inFlightDownloaded = 0;  // The received part of m_inFlightTotal.
    NodeErrorCode m_error = NodeErrorCode::NoError;
  };
  TerrainFusion GetTerrainFusion(CountryId const & countryId) const;
  StatusAndError GetEffectiveStatus(StatusAndError const & mapStatus, TerrainFusion const & terrain) const;

  // The terrain items ride the shared downloader queue but must not reach the Storage
  // subscriber (its notification paths walk the country tree and the block names are
  // not tree ids), so they subscribe to this dedicated forwarder instead.
  class TerrainQueueSubscriber : public QueuedCountry::Subscriber
  {
  public:
    explicit TerrainQueueSubscriber(Storage & storage) : m_storage(storage) {}

    void OnCountryInQueue(QueuedCountry const & queuedCountry) override;
    void OnStartDownloading(QueuedCountry const & queuedCountry) override;
    void OnDownloadProgress(QueuedCountry const & queuedCountry, downloader::Progress const & progress) override;
    void OnDownloadFinished(QueuedCountry const & queuedCountry, downloader::DownloadStatus status) override;

  private:
    Storage & m_storage;
  };

  struct TerrainBlockState
  {
    uint64_t m_bytesDownloaded = 0;
    uint64_t m_lastNotifiedBytes = 0;
  };

  // All GUI-thread-only: the in-flight blocks, the failures of the last batch and the
  // regions interested in each block (for the observer notifications).
  TerrainQueueSubscriber m_terrainSubscriber{*this};
  std::map<std::string, TerrainBlockState> m_terrainQueue;
  struct TerrainFailure
  {
    NodeErrorCode m_error = NodeErrorCode::UnknownError;
    bool m_retryable = false;
  };
  std::map<std::string, TerrainFailure> m_terrainFailures;
  std::map<std::string, std::set<CountryId>> m_terrainBlockRegions;

  TerrainBlock * FindTerrainBlock(std::string const & name);
  std::string GetTerrainDir(int64_t version) const;
  // The downloader's target path of the block (see QueuedCountry::GetFileDownloadPath).
  std::string GetTerrainReadyPath(TerrainBlock const & block) const;
  void OnTerrainBlockProgress(std::string const & name, downloader::Progress const & progress);
  void OnTerrainBlockDownloaded(QueuedCountry const & queuedCountry, downloader::DownloadStatus status);
  void NotifyTerrainRegions(std::string const & name);
  // The downloaded regions the failed blocks' interest points at: derived from the live
  // state at both the retry arming and the retry firing, so a cancel or a delete in
  // between (both empty the interest) mutes the retry.
  CountriesSet GetFailedTerrainRegions(bool retryableOnly) const;
  // The failed blocks auto-retry like the failed maps (DownloadingPolicy::ScheduleRetry)
  // for the downloaded regions still interested in them.
  void ScheduleTerrainRetry();

  // This function is called each time all files for a
  // country are deleted.
  DeleteCallback m_willDelete;

  // If |m_dataDir| is not empty Storage will create version directories and download maps in
  // platform::WritableDir/|m_dataDir|/. Not empty |m_dataDir| can be used only for
  // downloading maps to a special place but not for continue working with them from this place.
  std::string m_dataDir;

  bool m_integrityValidationEnabled = true;

  // |m_downloadMapOnTheMap| is called when an end user clicks on download map or retry button
  // on the map.
  DownloadFn m_downloadMapOnTheMap;

  CountryNameGetter m_countryNameGetter;

  CountriesInfo m_countriesInfo;

  ThreadChecker m_threadChecker;

  bool m_needToStartDeferredDownloading = false;

  StartDownloadingCallback m_startDownloadingCallback;

  DownloadingCountries m_downloadingCountries;

  void LoadCountriesFile(std::string const & pathToCountriesFile);

  void ReportProgress(CountryId const & countryId, downloader::Progress const & p);
  void ReportProgressForHierarchy(CountryId const & countryId);

  // QueuedCountry::Subscriber overrides:
  void OnCountryInQueue(QueuedCountry const & queuedCountry) override;
  void OnStartDownloading(QueuedCountry const & queuedCountry) override;
  /// Called on the main thread by MapFilesDownloader when
  /// downloading of a map file succeeds/fails.
  void OnDownloadFinished(QueuedCountry const & queuedCountry, downloader::DownloadStatus status) override;

  /// Periodically called on the main thread by MapFilesDownloader
  /// during the downloading process.
  void OnDownloadProgress(QueuedCountry const & queuedCountry, downloader::Progress const & progress) override;

  void RegisterDownloadedFiles(CountryId const & countryId, MapFileType type);

  void OnMapDownloadFinished(CountryId const & countryId, downloader::DownloadStatus status, MapFileType type);

  /// Dummy ctor for private use only.
  explicit Storage(int);

public:
  ThreadChecker const & GetThreadChecker() const { return m_threadChecker; }

  /// \brief Storage will create its directories in Writable Directory
  /// (gotten with platform::WritableDir) by default.
  /// \param pathToCountriesFile is a name of countries.json file.
  /// \param dataDir If |dataDir| is not empty Storage will create its directory in WritableDir/|dataDir|.
  /// \note if |dataDir| is not empty the instance of Storage can be used only for downloading map files
  /// but not for continue working with them.
  /// If |dataDir| is not empty the work flow is
  /// * create a instance of Storage with a special countries.json and |dataDir|
  /// * download some maps to WritableDir/|dataDir|
  /// * destroy the instance of Storage and move the downloaded maps to proper place
  Storage(std::string const & pathToCountriesFile = COUNTRIES_FILE, std::string const & dataDir = std::string());

  /// \brief This constructor should be used for testing only.
  Storage(std::string const & referenceCountriesTxtJsonForTesting,
          std::unique_ptr<MapFilesDownloader> mapDownloaderForTesting);

  void Init(UpdateCallback didDownload, DeleteCallback willDelete);

  /// Terrain (.twm) downloading: the TerrainProvider hooks injected by the Framework
  /// ("block landed" registration, the older-coverage probe, the blocks deletion).
  void SetTerrainCallbacks(TerrainDownloadedFn onDownloaded, TerrainHasOlderFn hasOlder, TerrainDeleteFn deleteFn = {});

  /// The tests exercising the fake map downloads must not bundle the terrain: the fake
  /// downloaders can not serve the blocks, and their landing needs the platform threads.
  void DisableTerrainForTesting();

  /// Enqueues the terrain blocks covering the country polygon (twm_grid.json "mwms")
  /// into the shared downloader queue; the blocks on disk or in flight are skipped. The
  /// blocks land into <writable>/terrain/<block version>/ after the countries.json-style
  /// integrity check; the observers get the region id notifications (no separate channel).
  void DownloadTerrain(CountryId const & countryId);

  /// Cancels the terrain blocks requested for the countryId subtree (the blocks another
  /// region still wants stay in the queue). CancelDownloadNode calls it too, so a country
  /// cancel drops both the maps and the terrain; this one drops the terrain only.
  void CancelTerrain(CountryId const & countryId);

  /// Deletes downloaded terrain no remaining downloaded or queued region needs. Shared
  /// blocks stay while at least one other region references their current-grid coverage.
  void DeleteTerrain(CountryId const & countryId);

  void SetDownloadingPolicy(DownloadingPolicy * policy);

  // Validates serverUrl and, if valid, makes it the sole map download server (persisted across
  // restarts). @param[out] normalizedUrl receives the canonical URL actually applied.
  // @returns false and changes nothing if serverUrl is not a valid http(s) base URL.
  bool SetDebugMapDownloadServer(std::string const & serverUrl, std::string & normalizedUrl);
  void ResetDebugMapDownloadServer();
  bool GetDebugMapDownloadServer(std::string & serverUrl) const;

  bool CheckFailedCountries(CountriesVec const & countries) const;

  /// @name Countries update functions. Public for unit tests.
  /// @{
  void RunCountriesCheckAsync();
  /// @return 0 If error.
  int64_t ParseIndexAndGetDataVersion(std::string const & index) const;
  void ApplyCountries(std::string const & countriesBuffer, Storage & storage);
  /// @}

  /// \brief Returns root country id of the country tree.
  CountryId const GetRootId() const;

  /// \param childIds is filled with children node ids by a parent. For example GetChildren(GetRootId())
  /// returns in param all countries ids. It's content of map downloader list by default.
  void GetChildren(CountryId const & parent, CountriesVec & childIds) const;

  /// \brief Fills |downloadedChildren| and |availChildren| with children of parent.
  /// If a direct child of |parent| contains at least one downloaded mwm
  /// the mwm id of the child will be added to |downloadedChildren|.
  /// If not, the mwm id the child will not be added to |availChildren|.
  /// \param parent is a parent acoording to countries.json or cournties_migrate.txt.
  /// \param downloadedChildren children partly or fully downloaded.
  /// \param availChildren fully available children. None of its files have been downloaded.
  /// \param keepAvailableChildren keeps all children in |availChildren| otherwise downloaded
  ///        children will be removed from |availChildren|.
  /// \note. This method puts to |downloadedChildren| and |availChildren| only real maps (and its ancestors)
  /// which have been written in coutries.txt or cournties_migrate.txt.
  /// It means the method does not put to its params neither custom maps generated by user
  /// nor World.mwm and WorldCoasts.mwm.
  void GetChildrenInGroups(CountryId const & parent, CountriesVec & downloadedChildren, CountriesVec & availChildren,
                           bool keepAvailableChildren = false) const;

  /// \brief Fills |nodes| with CountryIds of topmost nodes for this |countryId|.
  /// \param level is distance from top level except root.
  /// For disputed territories all possible owners will be added.
  /// Puts |countryId| to |nodes| when |level| is greater than the level of |countryId|.
  void GetTopmostNodesFor(CountryId const & countryId, CountriesVec & nodes, size_t level = 0) const;

  /// \brief Returns topmost country id prior root id or |countryId| itself, if it's already
  /// a topmost node or disputed territory id if |countryId| is a disputed territory or belongs to
  /// disputed territory.
  CountryId GetTopmostParentFor(CountryId const & countryId) const;
  /// \brief Returns parent id for node if node has single parent. Otherwise (if node is disputed
  /// territory and has multiple parents or does not exist) returns empty CountryId
  CountryId GetParentIdFor(CountryId const & countryId) const;

  /// \brief Returns current version for mwms which are used by storage.
  inline int64_t GetCurrentDataVersion() const { return m_currentVersion; }

  /// \brief Returns true if the node with countryId has been downloaded and false othewise.
  /// If countryId is expandable returns true if all mwms which belongs to it have downloaded.
  /// Returns false if countryId is an unknown string.
  /// \note The method return false for custom maps generated by user
  /// and World.mwm and WorldCoasts.mwm.
  bool IsNodeDownloaded(CountryId const & countryId) const;

  /// \brief Returns true if the last version of countryId has been downloaded.
  bool HasLatestVersion(CountryId const & countryId) const;

  /// \brief Returns true if the version of countryId can be used to update maps.
  bool IsAllowedToEditVersion(CountryId const & countryId) const;

  /// Returns version of downloaded mwm or zero.
  int64_t GetVersion(CountryId const & countryId) const;

  /// \brief Gets all the attributes for a node by its |countryId|.
  /// \param |nodeAttrs| is filled with attributes in this method.
  void GetNodeAttrs(CountryId const & countryId, NodeAttrs & nodeAttrs) const;

  /// \brief Gets a short list of node attributes by its |countriId|.
  /// \note This method works quicklier than GetNodeAttrs().
  void GetNodeStatuses(CountryId const & countryId, NodeStatuses & nodeStatuses) const;

  std::string GetNodeLocalName(CountryId const & countryId) const { return m_countryNameGetter(countryId); }

  /// \brief Downloads/update one node (expandable or not) by countryId.
  /// If node is expandable downloads/update all children (grandchildren) by the node
  /// until they haven't been downloaded before.
  void DownloadNode(CountryId const & countryId, bool isUpdate = false);

  /// \brief Delete node with all children (expandable or not).
  void DeleteNode(CountryId const & countryId);

  /// \brief Updates one node. It works for leaf and group mwms.
  /// \note If you want to update all the maps and this update is without changing
  /// borders or hierarchy just call UpdateNode(GetRootId()).
  void UpdateNode(CountryId const & countryId);

  /// \brief If the downloading a new node is in process cancels downloading the node and deletes
  /// the downloaded part of the map. If the map is in queue, remove the map from the queue.
  /// If the downloading a updating map is in process cancels the downloading,
  /// deletes the downloaded part of the map and leaves as is the old map (before the update)
  /// had been downloaded. It works for leaf and for group mwms.
  void CancelDownloadNode(CountryId const & countryId);

  /// \brief Downloading process could be interupted because of bad internet connection
  /// and some other reason.
  /// In that case user could want to recover it. This method is done for it.
  /// This method works with leaf and group mwm.
  /// In case of a group mwm this method retries downloading all mwm in m_failedCountries list
  /// which in the subtree with root |countryId|.
  /// It means the call RetryDownloadNode(GetRootId()) retries all the failed mwms.
  void RetryDownloadNode(CountryId const & countryId);

  struct UpdateInfo
  {
    // The regions with an update: the outdated maps plus the downloaded regions whose
    // terrain has blocks to fetch (the update badges count regions, not files).
    MwmCounter m_numberOfMwmFilesToUpdate = 0;

    // The sizes cover BOTH the mwm files and the missing terrain blocks of the
    // downloaded regions of the subtree - the update downloads both (see UpdateNode).
    MwmSize m_maxFileSizeInBytes = 0;
    MwmSize m_totalDownloadSizeInBytes = 0;

    // Difference size in bytes between before update and after update.
    int64_t m_sizeDifference = 0;
  };

  /// \brief Get information for the update button: what UpdateNode(countryId) would download.
  /// \return true if updateInfo is filled correctly and false otherwise.
  bool GetUpdateInfo(CountryId const & countryId, UpdateInfo & updateInfo) const;

  /// @name This functions should be called from 'main' thread only to avoid races.
  /// @{
  /// @return Pointer that will be stored for later use.
  Affiliations const * GetAffiliations() const;
  CountryNameSynonyms const & GetCountryNameSynonyms() const;
  /// @}

  /// For each node with \a root subtree (including).
  template <class ToDo>
  void ForEachInSubtree(CountryId const & root, ToDo && toDo) const;
  template <class ToDo>
  void ForEachAncestorExceptForTheRoot(CountryId const & childId, ToDo && toDo) const;
  template <class ToDo>
  /// For each leaf country excluding Worlds.
  void ForEachCountry(ToDo && toDo) const;

  /// \brief Sets callback which will be called in case of a click on download map button on the map.
  void SetCallbackForClickOnDownloadMap(DownloadFn & downloadFn);

  /// \brief Calls |m_downloadMapOnTheMap| if one has been set.
  /// \param |countryId| is country id of a leaf. That means it's a file name.
  /// \note This method should be called for a click of download map button
  /// and for a click for retry downloading map button on the map.
  void DoClickOnDownloadMap(CountryId const & countryId);
  //@}

  /// \returns real (not fake) local maps contained in countries.json.
  /// So this method does not return custom user local maps and World and WorldCoasts country id.
  // void GetLocalRealMaps(CountriesVec & localMaps) const;

  /// Do we have downloaded countries
  bool HaveDownloadedCountries() const;

  /// Delete local maps and aggregate their Id if needed
  void DeleteAllLocalMaps(CountriesVec * existedCountries = nullptr);

  // Clears local files registry and downloader's queue.
  void Clear();

  /// Used in Android to get absent Worlds files to download.
  /// @param[out] res Out vector, empty if all files are present some or error occured.
  /// @return     WorldStatus:
  enum class WorldStatus
  {
    READY = 0,            ///< Ready to download or all files are present if \a res is empty
    WAS_MOVED,            ///< All World files are present and one or more files was moved, \a res is empty.
    ERROR_CREATE_FOLDER,  ///< Error when creating folder
    ERROR_MOVE_FILE       ///< Error when trying to move World file
  };
  WorldStatus GetForceDownloadWorlds(std::vector<platform::CountryFile> & res) const;

  // Finds and registers all map files in maps directory. In the case
  // of several versions of the same map keeps only the latest one, others
  // are deleted from disk.
  // *NOTE* storage will forget all already known local maps.
  void RegisterAllLocalMaps(bool enableDiffs = false);

  // Returns list of all local maps, including fake countries (World*.mwm).
  void GetLocalMaps(std::vector<LocalFilePtr> & maps) const;

  // Returns number of downloaded maps (files), excluding fake countries (World*.mwm).
  size_t GetDownloadedFilesCount() const;

  /// Guarantees that change and progress are called in the main thread context.
  /// @return unique identifier (>0) that should be used with Unsubscribe function
  int Subscribe(ChangeCountryFunction change, ProgressFunction progress);
  void Unsubscribe(int slotId);

  /// Returns fused map and deduplicated terrain progress for watched leaf countries.
  downloader::Progress GetOverallProgress(CountriesVec const & countries) const;
  /// Bytes required to download the not-yet-downloaded leaf countries, with shared
  /// terrain blocks counted once.
  MwmSize GetDownloadSize(CountriesVec const & countries) const;

  Country const & CountryLeafByCountryId(CountryId const & countryId) const;
  Country const & CountryByCountryId(CountryId const & countryId) const;

  /// @todo Proxy functions for future, to distinguish CountryId from regular file name.
  /// @{
  CountryId const & FindCountryId(platform::LocalCountryFile const & localFile) const
  {
    return localFile.GetCountryName();
  }
  CountryId const & FindCountryIdByFile(std::string const & name) const { return name; }
  /// @}

  // Returns true iff |countryId| exists as a node in the tree.
  bool IsNode(CountryId const & countryId) const;

  /// @return true iff \a countryId is a leaf of the tree.
  bool IsLeaf(CountryId const & countryId) const;

  // Returns true iff |countryId| is an inner node of the tree.
  bool IsInnerNode(CountryId const & countryId) const;

  LocalAndRemoteSize CountrySizeInBytes(CountryId const & countryId) const;
  MwmSize GetRemoteSize(platform::CountryFile const & file) const;
  platform::CountryFile const & GetCountryFile(CountryId const & countryId) const;
  LocalFilePtr GetLatestLocalFile(CountryId const & countryId) const;

  /// Slow version, but checks if country is out of date
  Status CountryStatusEx(CountryId const & countryId) const;

  /// Puts country denoted by countryId into the downloader's queue.
  /// During downloading process notifies observers about downloading
  /// progress and status changes.
  void DownloadCountry(CountryId const & countryId, MapFileType type);

  /// Removes country files (for all versions) from the device.
  /// Notifies observers about country status change.
  void DeleteCountry(CountryId const & countryId, MapFileType type);

  bool DeleteFakeCountry(CountryId const & countryId);

  /// Removes country files of a particular version from the device.
  /// Notifies observers about country status change.
  void DeleteCustomCountryVersion(platform::LocalCountryFile const & localFile);

  bool IsDownloadInProgress() const;

  /// @param[out] res Populated with oudated countries.
  // void GetOutdatedCountries(std::vector<Country const *> & countries) const;

  /// Sets and gets locale, which is used to get localized counries names
  void SetLocale(std::string const & locale) { m_countryNameGetter.SetLocale(locale); }
  std::string GetLocale() const { return m_countryNameGetter.GetLocale(); }

  // for testing:
  void SetEnabledIntegrityValidationForTesting(bool enabled);
  void SetDownloaderForTesting(std::unique_ptr<MapFilesDownloader> downloader);
  void SetCurrentDataVersionForTesting(int64_t currentVersion);
  void SetDownloadingServersForTesting(std::vector<std::string> const & downloadingUrls);
  void SetLocaleForTesting(std::string const & jsonBuffer, std::string const & locale);

  /// Returns true if the diff scheme is available and all local outdated maps can be updated via diffs.
  // bool IsPossibleToAutoupdate() const;

  void SetStartDownloadingCallback(StartDownloadingCallback const & cb);

  std::string GetFilePath(CountryId const & countryId, MapFileType file) const;

  void RestoreDownloadQueue();

protected:
  void OnFinishDownloading();

private:
  friend struct storage_tests::UnitClass_StorageTest_DeleteCountry;

  void SaveDownloadQueue();

  // Returns true when country is in the downloader's queue.
  bool IsCountryInQueue(CountryId const & countryId) const;

  // Returns true if we started the diff applying procedure for an mwm with countryId.
  bool IsDiffApplyingInProgressToCountry(CountryId const & countryId) const;

  // Returns local country files of a particular version, or wrapped
  // nullptr if there're no country files corresponding to the
  // version.
  LocalFilePtr GetLocalFile(CountryId const & countryId, int64_t version) const;

  // Tries to register disk files for a real (listed in countries.json)
  // country. If map files of the same version were already
  // registered, does nothing.
  void RegisterCountryFiles(LocalFilePtr localFile);

  // Registers disk files for a country. This method must be used only
  // for real (listed in countries.json) countries.
  void RegisterLocalFile(platform::LocalCountryFile const & localFile);

  // Removes disk files for all versions of a country.
  void DeleteCountryFiles(CountryId const & countryId, MapFileType type, bool deferredDelete);

  // Removes country files from downloader.
  bool DeleteCountryFilesFromDownloader(CountryId const & countryId);

  // Returns a path to a place on disk downloader can use for downloaded files.
  std::string GetFileDownloadPath(CountryId const & countryId, MapFileType file) const;

  /// Fast version, doesn't check if country is out of date
  Status CountryStatus(CountryId const & countryId) const;

  /// Returns status for a node (group node or not).
  StatusAndError GetNodeStatus(CountryTree::Node const & node) const;

  /// Returns status for a node (group node or not).
  /// Fills |disputedTeritories| with all disputed teritories in subtree with the root == |node|.
  StatusAndError GetNodeStatusInfo(CountryTree::Node const & node,
                                   std::vector<std::pair<CountryId, NodeStatus>> & disputedTeritories,
                                   bool isDisputedTerritoriesCounted) const;

  void NotifyStatusChanged(CountryId const & countryId);
  void NotifyStatusChangedForHierarchy(CountryId const & countryId);

  /// Calculates progress of downloading for expandable nodes in country tree.
  downloader::Progress CalculateProgress(CountryTree::Node const & subtreeRoot, CountriesSet const & mwmsInQueue) const;

  template <class ToDo>
  void ForEachAncestorExceptForTheRoot(CountryTree::NodesBufferT const & nodes, ToDo && toDo) const;

  /// @return true if |node.Value().Name()| is a disputed territory and false otherwise.
  bool IsDisputed(CountryTree::Node const & node) const;

  /// @return true iff \a node is a country MWM leaf of the tree.
  static bool IsCountryLeaf(CountryTree::Node const & node);
  static bool IsWorldCountryID(CountryId const & country);

  void OnMapDownloadFailed(CountryId const & countryId);

  // void LoadDiffScheme();
  void ApplyDiff(CountryId const & countryId, std::function<void(bool isSuccess)> const & fn);

  using IsDiffAbsentForCountry = std::function<bool(CountryId const & id)>;
  void SetMapSchemeForCountriesWithAbsentDiffs(IsDiffAbsentForCountry const & isAbsent);
  void AbortDiffScheme();

  // Should be called once on startup, downloading process should be suspended until this method
  // was not called. Do not call this method manually.
  void OnDiffStatusReceived(diffs::NameDiffInfoMap && diffs);
};

CountriesSet GetQueuedCountries(QueueInterface const & queue);

template <class ToDo>
void Storage::ForEachInSubtree(CountryId const & root, ToDo && toDo) const
{
  CountryTree::Node const * const rootNode = m_countries.FindFirst(root);
  if (rootNode == nullptr)
  {
    ASSERT(false, ("CountryId =", root, "not found in m_countries."));
    return;
  }
  rootNode->ForEachInSubtree([&toDo](CountryTree::Node const & node)
  {
    Country const & value = node.Value();
    toDo(value.Name(), value.GetSubtreeMwmCounter() != 1 /* groupNode. */);
  });
}

/// Calls functor |toDo| with signature
/// void(const CountryId const & parentId, CountriesVec const & descendantCountryId)
/// for each ancestor except for the main root of the tree in order from the leaf to the root.
/// Note. In case of disputable territories several nodes with the same name may be
/// present in the country tree. In that case ForEachAncestorExceptForTheRoot calls
/// |toDo| for parents of each way to the root in the country tree. In case of diamond
/// trees toDo is called for common part of ways to the root only once.
template <class ToDo>
void Storage::ForEachAncestorExceptForTheRoot(CountryId const & countryId, ToDo && toDo) const
{
  CountryTree::NodesBufferT nodes;
  m_countries.Find(countryId, nodes);
  if (nodes.empty())
  {
    ASSERT(false, ("CountryId =", countryId, "not found in m_countries."));
    return;
  }

  ForEachAncestorExceptForTheRoot(nodes, std::forward<ToDo>(toDo));
}

template <class ToDo>
void Storage::ForEachAncestorExceptForTheRoot(CountryTree::NodesBufferT const & nodes, ToDo && toDo) const
{
  // In most cases nodes.size() == 1, so a small inline buffer avoids heap allocation.
  buffer_vector<CountryTree::Node const *, 8> visitedAncestors;
  for (auto const & node : nodes)
  {
    node->ForEachAncestorExceptForTheRoot([&](CountryTree::Node const & node)
    {
      if (std::find(visitedAncestors.begin(), visitedAncestors.end(), &node) != visitedAncestors.end())
        return;  // The node was visited before because countryId is present in the tree more
                 // than once.
      visitedAncestors.push_back(&node);
      toDo(node.Value().Name(), node);
    });
  }
}

template <class ToDo>
void Storage::ForEachCountry(ToDo && toDo) const
{
  m_countries.GetRoot().ForEachInSubtree([&](CountryTree::Node const & node)
  {
    if (IsCountryLeaf(node))
      toDo(node.Value().GetFile());
  });

  for (auto const & [country, _] : m_localFilesForFakeCountries)
    toDo(country);
}
}  // namespace storage
