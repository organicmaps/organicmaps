# A. Split BookmarkManager into Focused Components

## Problem

BookmarkManager is a god object (~835-line header at `libs/map/bookmark_manager.hpp`, ~3676-line impl at `libs/map/bookmark_manager.cpp`) with 100+ methods mixing unrelated concerns:

- **CRUD** for bookmarks, tracks, categories, user marks, compilations
- **File I/O** (load/save/import/export in KML/KMZ/GPX/GeoJSON)
- **Sorting** (4 algorithms with async support and address geocoding)
- **Elevation profile** management & track selection state
- **Drape engine** communication (rendering updates, symbol sizes)
- **Search indexing**, address geocoding
- **Metadata persistence** (`bm.json` for sorting prefs, custom properties)
- **Trash/recovery** system (soft delete to `.Trash/`, restore)
- **Route saving**, symbol size management

Impact: hard to understand, test, and modify. Every change risks unintended side effects.

## Proposed Split

### 1. BookmarkSorter (extract first -- clearest boundary)

**Extracted methods** (from bookmark_manager.cpp):
- `GetSortedCategory()` (lines 1530-1607)
- `GetSortedCategoryImpl()` (lines 1512-1528)
- `SortByDistance()` (lines 1309-1372)
- `SortByTime()` (lines 1374-1417)
- `SortByType()` (lines 1419-1486)
- `SortByName()` (lines 1488-1510)
- `PrepareBookmarksAddresses()` (lines 1222-1236)
- `FilterInvalidData()` (lines 1238-1253)
- `SetBookmarksAddresses()` (lines 1255-1265)
- `AddTracksSortedBlock()` (lines 1268-1280)
- `SortTracksByTime()` (lines 1283-1300)
- `SortTracksByName()` (lines 1303-1307)
- `GetAvailableSortingTypes()` (lines 698-757)
- `GetLastSortingType()` / `SetLastSortingType()` / `ResetLastSortingType()`
- Metadata: `SaveMetadata()` (lines 1968-1992), `LoadMetadata()` (lines 1994-2027), `CleanupInvalidMetadata()` (lines 1948-1966)

**Extracted state:**
- `m_regionAddressGetter` + `m_regionAddressMutex`
- `m_metadata` (Metadata struct)
- `SortBookmarkData`, `SortTrackData` structs (lines 659-689)

**Interface:**
```cpp
class BookmarkSorter
{
public:
  // Takes a reference to BookmarkManager for data access
  explicit BookmarkSorter(BookmarkManager & bmManager);

  void InitRegionAddressGetter(DataSource const &, storage::CountryInfoGetter const &);
  std::string GetLocalizedRegionAddress(m2::PointD const & pt);

  std::vector<BookmarkManager::SortingType> GetAvailableSortingTypes(
    kml::MarkGroupId groupId, bool hasMyPosition) const;
  void GetSortedCategory(BookmarkManager::SortParams const & params);

  bool GetLastSortingType(kml::MarkGroupId, BookmarkManager::SortingType &) const;
  void SetLastSortingType(kml::MarkGroupId, BookmarkManager::SortingType);
  void ResetLastSortingType(kml::MarkGroupId);

  void PrepareForSearch(kml::MarkGroupId groupId);

  void LoadMetadata();
  void SaveMetadata();

  void EnableTestMode(bool enable);

private:
  BookmarkManager & m_bmManager;
  std::unique_ptr<search::RegionAddressGetter> m_regionAddressGetter;
  std::mutex m_regionAddressMutex;
  BookmarkManager::Metadata m_metadata;
  bool m_testModeEnabled = false;
  // ... sort helpers
};
```

### 2. BookmarkFileManager (extract second)

**Extracted methods:**
- `LoadBookmarks()` public (lines 2053-2075) and private overload (lines 2029-2051)
- `LoadBookmark()` (lines 2077-2089)
- `ReloadBookmark()` (lines 2091-2101)
- `LoadBookmarkRoutine()` (lines 2103-2157)
- `ReloadBookmarkRoutine()` (lines 2159-2191)
- `NotifyAboutStartAsyncLoading()` (lines 2193-2204)
- `NotifyAboutFinishAsyncLoading()` (lines 2206-2242)
- `NotifyAboutFile()` (line 2228)
- `SaveBookmarks()` (lines 2961-2985)
- `PrepareToSaveBookmarks()` (lines 2929-2959)
- `PrepareToSaveBookmarksForTrack()` (lines 2914-2927)
- `SaveBookmarkCategory()` both overloads (lines 2895-2912)
- `CollectBmGroupKMLData()` (lines 2867-2893)
- `PrepareFileForSharing()` (lines 3004-3032)
- `PrepareTrackFileForSharing()` (lines 2987-3002)
- `PrepareAllFilesForSharing()` (lines 3034-3040)
- Trash: `GetRecentlyDeletedCategories*()`, `RecoverRecentlyDeleted*()`, `DeleteRecentlyDeleted*()` (lines 423-468)

**Extracted state:**
- `m_asyncLoadingCallbacks`
- `m_asyncLoadingInProgress`, `m_loadBookmarksCalled`, `m_loadBookmarksFinished`
- `m_bookmarkLoadingQueue` (std::list<BookmarkLoaderInfo>)
- `m_needTeardown` (atomic)

**Interface:**
```cpp
class BookmarkFileManager
{
public:
  explicit BookmarkFileManager(BookmarkManager & bmManager);

  void SetAsyncLoadingCallbacks(BookmarkManager::AsyncLoadingCallbacks && callbacks);
  bool IsAsyncLoadingInProgress() const;

  void LoadBookmarks();
  void LoadBookmark(std::string const & filePath, bool isTemporaryFile);
  void ReloadBookmark(std::string const & filePath);

  void SaveBookmarks(kml::GroupIdCollection const & groupIdCollection);
  bool SaveBookmarkCategory(kml::MarkGroupId groupId);
  bool SaveBookmarkCategory(kml::MarkGroupId groupId, Writer & writer, FileType fileType) const;

  void PrepareFileForSharing(kml::GroupIdCollection && ids, BookmarkManager::SharingHandler && handler, FileType ft);
  void PrepareTrackFileForSharing(kml::TrackId trackId, BookmarkManager::SharingHandler && handler, FileType ft);
  void PrepareAllFilesForSharing(BookmarkManager::SharingHandler && handler);

  size_t GetRecentlyDeletedCategoriesCount() const;
  BookmarkManager::KMLDataCollectionPtr GetRecentlyDeletedCategories();
  bool IsRecentlyDeletedCategory(std::string const & filePath) const;
  void RecoverRecentlyDeletedCategoriesAtPaths(std::vector<std::string> const & filePaths);
  void DeleteRecentlyDeletedCategoriesAtPaths(std::vector<std::string> const & filePaths);

  void Teardown();
  void EnableTestMode(bool enable);

private:
  BookmarkManager & m_bmManager;
  // ... state
};
```

### 3. TrackSelectionManager (extract third)

**Extracted methods:**
- `SetElevationActivePoint()`, `GetElevationActivePoint()` (lines 904-930)
- `UpdateElevationMyPosition()`, `GetElevationMyPosition()` (lines 932-952)
- `FindNearestTrack()`, `GetTrackSelectionInfo()` (lines 1078-1220)
- `SetTrackSelectionInfo()` (lines 1024-1044)
- `OnTrackSelected()`, `OnTrackDeselected()` (lines 1046-1076)
- `GetTrackSelectionMarkId()` (lines 954-966)
- `SetTrackSelectionMark()`, `DeleteTrackSelectionMark()`, `ResetTrackInfoMark()` (lines 977-1022)
- `UpdateTrackMarksMinZoom()`, `UpdateTrackMarksVisibility()` (lines 1804-1820)

**Extracted state:**
- `m_selectedTrackId`
- `m_trackInfoMarkId`
- `m_elevationActivePointChanged`, `m_elevationMyPositionChanged` callbacks
- `m_lastElevationMyPosition`

### 4. BookmarkManager becomes thin facade

After extraction, BookmarkManager retains:
- Data ownership (`m_categories`, `m_bookmarks`, `m_tracks`, `m_userMarks`, `m_userMarkLayers`)
- EditSession and change tracking
- Drape engine integration (until plan C extracts it)
- `GetEditSession()`, query methods, `CreateCategories()`
- Delegation to sub-components

## Files to Create

| File | Content |
|------|---------|
| `libs/map/bookmark_sorter.hpp` | BookmarkSorter class + Metadata struct |
| `libs/map/bookmark_sorter.cpp` | ~400 lines of sorting implementation |
| `libs/map/bookmark_file_manager.hpp` | BookmarkFileManager class |
| `libs/map/bookmark_file_manager.cpp` | ~500 lines of I/O implementation |
| `libs/map/track_selection_manager.hpp` | TrackSelectionManager class |
| `libs/map/track_selection_manager.cpp` | ~300 lines of track selection |

## Files to Modify

| File | Changes |
|------|---------|
| `libs/map/bookmark_manager.hpp` | Remove extracted methods/state, add component members |
| `libs/map/bookmark_manager.cpp` | Remove extracted implementations, add delegation |
| `libs/map/CMakeLists.txt` | Add 6 new source files |

## Tests

### Existing tests (must pass unchanged)
- `libs/map/map_tests/bookmarks_test.cpp` - Uses BookmarkManager public API, facade preserves compatibility

### New tests
- `libs/map/map_tests/bookmark_sorter_test.cpp`:
  - SortByName with empty collection
  - SortByTime with mixed timestamps
  - SortByType with type counts below kMinCommonTypesCount
  - SortByDistance grouping by region
  - GetAvailableSortingTypes for various category compositions
  - Metadata save/load roundtrip
  - Metadata cleanup of invalid entries

- `libs/map/map_tests/bookmark_file_manager_test.cpp`:
  - LoadBookmarks scans directory and creates categories
  - LoadBookmark queues when async in progress
  - SaveBookmarks writes KML files
  - PrepareFileForSharing creates temp file
  - Trash system: delete -> recover roundtrip
  - File format detection (KML, KMZ, GPX, GeoJSON)

- `libs/map/map_tests/track_selection_test.cpp`:
  - SetElevationActivePoint updates mark position
  - FindNearestTrack returns correct track
  - OnTrackSelected/Deselected toggle visibility
  - UpdateElevationMyPosition with position on/off track

## Implementation Steps

1. Create `BookmarkSorter` class with all sorting methods
2. Add `BookmarkSorter` as member of `BookmarkManager`
3. Replace sorting method bodies in BookmarkManager with delegation
4. Verify all tests pass
5. Repeat for `BookmarkFileManager`
6. Repeat for `TrackSelectionManager`
7. Clean up BookmarkManager (remove dead includes, reorder members)

## Risks & Mitigations

- **EditSession interactions**: Session close triggers `NotifyChanges()` which triggers auto-save via FileManager. Mitigation: FileManager exposes `SaveDirtyCategories()` called from `NotifyChanges()`.
- **Circular dependencies**: Components need BookmarkManager for data access, BookmarkManager owns components. Mitigation: Components take `BookmarkManager &` in constructor, no header-level circular includes (forward declarations suffice).
- **Test mode**: Multiple components have `m_testModeEnabled`. Mitigation: Single flag on BookmarkManager, components query it via reference.

## Priority
Phase 2 (after dead code cleanup and EditSession simplification). Start with BookmarkSorter extraction.
