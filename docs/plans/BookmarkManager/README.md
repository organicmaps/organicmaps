# BookmarkManager Architecture Analysis & Improvement Plans

## Overview

This plan creates detailed implementation plans for each BookmarkManager improvement, to be written as individual files in `docs/plans/BookmarkManager/`. Each plan includes context, detailed steps, files to modify, and tests.

Upon approval, the following files will be created:

```
docs/plans/BookmarkManager/
  A-split-into-focused-components.md
  B-unify-change-notification.md
  C-extract-drape-communication.md
  D-simplify-edit-session.md
  E-unified-platform-bridge.md
  F-separate-system-marks.md
  G-async-data-access-android.md
  H-dead-code-cleanup.md
```

---

## A. Split BookmarkManager into Focused Components

### Problem
BookmarkManager is a god object (~835-line header, ~3676-line impl) with 100+ methods mixing:
- CRUD for bookmarks/tracks/categories/user marks/compilations
- File I/O (load/save/import/export in KML/KMZ/GPX/GeoJSON)
- Sorting (4 algorithms with async support)
- Elevation profile management & track selection
- Drape engine communication
- Search indexing, address geocoding
- Metadata persistence (`bm.json`)
- Trash/recovery system
- Route saving, symbol size management

### Proposed Split

**BookmarkStore** (data ownership + CRUD):
- Owns `m_categories`, `m_bookmarks`, `m_tracks`, `m_userMarks`, `m_userMarkLayers`
- CreateBookmark/Track/Category, Delete, Update, Move, Attach/Detach
- Query methods: GetBookmark, GetTrack, GetUserMarkIds, GetTrackIds, HasBmCategory, etc.
- EditSession lives here (it's fundamentally about batching data mutations)
- ~40 methods, ~1500 lines

**BookmarkFileManager** (file I/O):
- `LoadBookmarks()`, `LoadBookmark()`, `ReloadBookmark()`
- `SaveBookmarks()`, `PrepareToSaveBookmarks()`, `CollectBmGroupKMLData()`
- `PrepareFileForSharing()`, `PrepareTrackFileForSharing()`, `PrepareAllFilesForSharing()`
- Loading queue (`m_bookmarkLoadingQueue`), async state (`m_asyncLoadingInProgress`)
- `AsyncLoadingCallbacks`
- Depends on BookmarkStore for data access
- ~15 methods, ~500 lines

**BookmarkSorter** (sorting):
- `GetSortedCategory()`, `GetSortedCategoryImpl()`
- `SortByDistance/Time/Type/Name()`
- `PrepareBookmarksAddresses()`, `FilterInvalidData()`, `SetBookmarksAddresses()`
- `GetAvailableSortingTypes()`
- Owns `m_regionAddressGetter` and `m_regionAddressMutex`
- Metadata for sorting preferences
- ~15 methods, ~400 lines

**TrackSelectionManager** (track selection + elevation):
- `SetElevationActivePoint()`, `GetElevationActivePoint()`
- `UpdateElevationMyPosition()`, `GetElevationMyPosition()`
- `FindNearestTrack()`, `GetTrackSelectionInfo()`, `SetTrackSelectionInfo()`
- `OnTrackSelected()`, `OnTrackDeselected()`
- Track selection marks management
- Owns `m_selectedTrackId`, elevation callbacks
- ~15 methods, ~300 lines

**BookmarkManager** becomes a **thin facade** delegating to these components, maintaining backward compatibility for platform bridges.

### Key Files to Modify
- `libs/map/bookmark_manager.hpp` - Split into 4 new headers + facade
- `libs/map/bookmark_manager.cpp` - Split into 4 new .cpp files + facade
- `libs/map/CMakeLists.txt` - Add new source files
- No platform bridge changes needed initially (facade preserves API)

### Tests
- Existing `libs/map/map_tests/bookmarks_test.cpp` should pass unchanged (facade preserves API)
- Add unit tests for each component in isolation:
  - `bookmark_store_test.cpp` - CRUD operations without file I/O
  - `bookmark_file_manager_test.cpp` - Load/save with mock store
  - `bookmark_sorter_test.cpp` - Sorting with mock data
  - `track_selection_test.cpp` - Selection/elevation with mock store

### Implementation Order
1. Extract `BookmarkSorter` first (clearest boundary, self-contained)
2. Extract `BookmarkFileManager` (clear I/O boundary)
3. Extract `TrackSelectionManager` (clear feature boundary)
4. What remains is `BookmarkStore` + facade

### Risk
- Low: facade preserves public API, no platform changes needed
- Medium: careful with `EditSession` interactions across components (session notifications must still trigger file saves)

---

## B. Unify Change Notification

### Problem
12+ callback mechanisms across 3 layers:
1. **Constructor callbacks** (5): created/updated/deleted/attached/detached bookmarks
2. **Settable callbacks** (4+): bookmarksChanged, categoriesChanged, elevation x2, symbolSizes
3. **AsyncLoading callbacks** (4): started, finished, fileSuccess, fileError
4. **3 parallel MarksChangesTrackers**: `m_changesTracker` -> merge into `m_bookmarksChangesTracker` + `m_drapeChangesTracker` -> reset all

The merge flow in `NotifyChanges()` (line 580-642 of bookmark_manager.cpp):
```
m_changesTracker.AcceptDirtyItems()
  -> Check HasBookmarksChanges/HasCategoriesChanges -> fire simple callbacks
  -> m_bookmarksChangesTracker.AddChanges(m_changesTracker)
  -> m_drapeChangesTracker.AddChanges(m_changesTracker)
  -> m_changesTracker.ResetChanges()
  -> if bookmarks changed: auto-save + SendBookmarksChanges (5 callbacks)
  -> m_bookmarksChangesTracker.ResetChanges()
  -> if drape changes: lock engine, update visibility/marks, invalidate
  -> m_drapeChangesTracker.ResetChanges()
```

### Proposed Solution

Replace 3 trackers with **2 trackers** (minimum viable simplification):
- `m_changesTracker` (accumulation during edit session) - keep as-is
- `m_notificationTracker` (merged, serves both persistence and drape)

The key insight: `m_bookmarksChangesTracker` and `m_drapeChangesTracker` always receive identical `AddChanges()` calls. The only reason they're separate is they're reset at different points. Instead, reset once after both consumers have processed.

```cpp
void BookmarkManager::NotifyChanges(bool saveChangesOnDisk)
{
  m_changesTracker.AcceptDirtyItems();
  if (!m_changesTracker.HasChanges() && !m_firstDrapeNotification)
    return;

  // Simple callbacks
  if (m_changesTracker.HasBookmarksChanges()) NotifyBookmarksChanged();
  if (m_changesTracker.HasCategoriesChanges()) NotifyCategoriesChanged();

  // Persistence
  if (m_changesTracker.HasBookmarksChanges())
  {
    // Auto-save
    if (saveChangesOnDisk) SaveDirtyCategories(m_changesTracker);
    // Platform callbacks
    SendBookmarksChanges(m_changesTracker);
  }

  // Drape
  UpdateDrape(m_changesTracker);

  m_changesTracker.ResetChanges();
}
```

### Unify Platform Callbacks

Replace 5 constructor callbacks + 2 settable callbacks with a single observer interface:

```cpp
class BookmarkObserver {
public:
  virtual ~BookmarkObserver() = default;
  virtual void OnBookmarksChanged() {}
  virtual void OnCategoriesChanged() {}
  virtual void OnBookmarksCreated(std::vector<BookmarkInfo> const &) {}
  virtual void OnBookmarksUpdated(std::vector<BookmarkInfo> const &) {}
  virtual void OnBookmarksDeleted(std::vector<kml::MarkId> const &) {}
  virtual void OnBookmarksAttached(std::vector<BookmarkGroupInfo> const &) {}
  virtual void OnBookmarksDetached(std::vector<BookmarkGroupInfo> const &) {}
};

void AddObserver(BookmarkObserver & observer);
void RemoveObserver(BookmarkObserver & observer);
```

### Key Files
- `libs/map/bookmark_manager.hpp` lines 51-101 (callbacks), 728-730 (trackers), 736-742 (settable callbacks)
- `libs/map/bookmark_manager.cpp` lines 580-642 (NotifyChanges), 2381-2426 (SendBookmarksChanges)
- `libs/map/framework.cpp` - where Callbacks struct is constructed

### Tests
- All existing bookmark tests must pass (they use the same public API)
- New test: verify observer receives correct events for each operation type
- New test: verify batching (nested EditSessions produce single notification)
- New test: verify drape tracker receives same changes as persistence path

### Risk
- Medium: NotifyChanges is the most critical method. Must preserve exact notification ordering.
- The `m_notificationsEnabled` flag and `m_firstDrapeNotification` logic must be preserved.

---

## C. Extract Drape Communication

### Problem
BookmarkManager directly communicates with the rendering engine:
- Owns `m_drapeEngine` (`DrapeEngineSafePtr`)
- `NotifyChanges()` locks Drape engine, updates visibility, pushes mark changes, invalidates
- `RequestSymbolSizes()` makes async call to Drape, handles response on GUI thread
- `UpdateBookmarksTextPlacement()` calls Drape directly
- `MarksChangesTracker` implements `df::UserMarksProvider` (Drape interface)
- `SetDrapeEngine()` lifecycle method triggers full sync

This couples data management to rendering. BookmarkManager can't be tested without a Drape engine (or null-checking it everywhere).

### Proposed Solution

Create `BookmarkRenderAdapter`:

```cpp
class BookmarkRenderAdapter : public BookmarkObserver
{
public:
  BookmarkRenderAdapter(BookmarkStore & store);

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);

  // BookmarkObserver overrides - receives change notifications
  void OnBookmarksChanged() override;
  // ... etc

  // Called by BookmarkManager when Drape needs update
  void UpdateUserMarks(MarksChangesTracker const & tracker);
  void UpdateBookmarksTextPlacement();

  void RequestSymbolSizes(OnSymbolSizesAcquiredCallback && callback);

private:
  BookmarkStore & m_store;
  df::DrapeEngineSafePtr m_drapeEngine;
  MarksChangesTracker m_drapeChangesTracker;
  bool m_firstNotification = false;
  m2::PointF m_maxBookmarkSymbolSize;
  bool m_symbolSizesAcquired = false;
};
```

### Key Files
- `libs/map/bookmark_manager.hpp` - Remove m_drapeEngine, m_drapeChangesTracker, drape methods
- `libs/map/bookmark_render_adapter.hpp` (new)
- `libs/map/bookmark_render_adapter.cpp` (new) - Move lines 623-649 from bookmark_manager.cpp
- `libs/map/framework.cpp` - Wire up adapter between BookmarkManager and DrapeEngine
- `libs/drape_frontend/drape_engine_safe_ptr.hpp` - No changes needed
- `libs/drape_frontend/user_marks_provider.hpp` - No changes needed

### Tests
- Existing tests pass (they use test mode which skips Drape)
- New test: BookmarkRenderAdapter receives changes and calls DrapeEngine mock
- New test: Symbol size callback flow

### Risk
- Medium: `NotifyChanges()` interleaves persistence and drape updates. Must ensure ordering is preserved.
- `m_firstDrapeNotification` logic needs careful migration.

### Dependency
- Should be done after B (Unify Change Notification), as it simplifies the tracker merging.

---

## D. Simplify Edit Session

### Problem
`EditSession` (lines 103-172 in header) is a forwarding wrapper with 30+ proxy methods that each delegate to the corresponding BookmarkManager private method. Every new mutation requires adding a method to both classes.

The only value EditSession provides: ref-counting `m_openedEditSessionsCount` to defer `NotifyChanges()` until the outermost session closes.

### Proposed Solution

**Option 1: Thin scope guard** (recommended):
```cpp
class EditSession
{
public:
  explicit EditSession(BookmarkManager & bm) : m_bm(bm) { m_bm.BeginBatch(); }
  ~EditSession() { m_bm.EndBatch(); }
  BookmarkManager & operator*() { return m_bm; }
  BookmarkManager * operator->() { return &m_bm; }
private:
  BookmarkManager & m_bm;
};

// Usage:
{
  auto session = bmManager.GetEditSession();
  session->CreateBookmark(std::move(data), groupId);
  session->SetCategoryName(catId, "New Name");
  // ~EditSession -> EndBatch -> NotifyChanges if outermost
}
```

This removes all 30+ proxy methods. The mutation methods move from private to public on BookmarkManager (they already were public via EditSession anyway).

**Impact on callers:** Minimal - change `session.Method()` to `session->Method()`. Search-and-replace.

### Key Files
- `libs/map/bookmark_manager.hpp` - Remove 30+ EditSession methods, make mutations public
- `libs/map/bookmark_manager.cpp` - Remove 30+ EditSession forwarding implementations (lines 3533-3676)
- All callers: `session.Foo()` -> `session->Foo()` (mechanical change)

### Tests
- All existing tests pass with syntax change
- New test: verify `operator->` works correctly
- New test: verify nested sessions still batch correctly

### Risk
- Low: mechanical refactor, no behavioral change
- Making mutations public is safe - they were already effectively public via EditSession

---

## E. Unified Platform Bridge (C++ Side)

### Problem
iOS and Android each maintain independent bridge layers with duplicated:
- Observer/listener management (NSHashTable vs Java listener lists)
- Data model wrappers (MWMBookmark/MWMTrack vs Bookmark.java/Track.java)
- Enum translations (color, file type, sorting type conversions)
- Callback registration boilerplate (4 async callbacks, 5 constructor callbacks)

Every new feature requires parallel changes in C++ core, iOS bridge (ObjC++), and Android bridge (JNI).

### Proposed Solution

Create a C++ `BookmarkBridge` class that provides a simplified, platform-neutral API:

```cpp
// Data transfer objects (no pointers, no C++ types in public API)
struct BookmarkDTO {
  int64_t id;
  int64_t categoryId;
  std::string name;
  std::string description;
  double lat, lon;
  int colorIndex;
  std::string iconName;
  std::string featureType;
  std::string address;
};

struct TrackDTO {
  int64_t id;
  int64_t categoryId;
  std::string name;
  double lengthMeters;
  uint32_t color; // ARGB
};

struct CategoryDTO {
  int64_t id;
  std::string name;
  std::string author;
  std::string description;
  size_t bookmarkCount;
  size_t trackCount;
  bool visible;
  int accessRules;
};

class BookmarkBridge {
public:
  // Observer token for RAII lifecycle management
  class ObserverToken { ... };

  // Register observer with automatic cleanup
  ObserverToken AddObserver(BookmarkBridgeObserver & observer);

  // Query (returns DTOs, no raw pointers)
  std::vector<CategoryDTO> GetCategories() const;
  std::optional<BookmarkDTO> GetBookmarkInfo(int64_t id) const;
  std::optional<TrackDTO> GetTrackInfo(int64_t id) const;

  // Mutations
  int64_t CreateCategory(std::string name);
  void DeleteCategory(int64_t id, bool permanently);
  // ... etc
};
```

### Key Files (new)
- `libs/map/bookmark_bridge.hpp` - Bridge API with DTOs
- `libs/map/bookmark_bridge.cpp` - Delegates to BookmarkManager

### Key Files (modified)
- `iphone/CoreApi/CoreApi/Bookmarks/MWMBookmarksManager.mm` - Simplify to use bridge
- `android/sdk/src/main/cpp/.../BookmarkManager.cpp` - Simplify JNI to use bridge

### Tests
- `bookmark_bridge_test.cpp` - Test all DTO conversions and observer lifecycle
- Existing platform tests should pass

### Risk
- High effort, but low risk per change (each method migration is independent)
- Can be done incrementally: migrate one method at a time

### Dependency
- Independent of A-D, can be done in parallel

---

## F. Separate System Marks

### Problem
BookmarkManager manages both user-created bookmarks AND ephemeral system marks:
- `StaticMarkPoint` (selection indicator) - created in constructor, stored as raw pointer `m_selectionMark`
- `MyPositionMarkPoint` (my position) - created in constructor, stored as raw pointer `m_myPositionMark`
- `TrackSelectionMark` (elevation profile cursor) - created/destroyed dynamically per track
- `TrackInfoMark` (track info popup) - single instance, stored via `m_trackInfoMarkId`
- Search marks, route marks, speed camera marks, etc. via `m_userMarkLayers`

These are fundamentally different: bookmarks are persistent/user-owned/file-backed; system marks are ephemeral/app-owned/never saved.

### Proposed Solution

Create `SystemMarkManager`:

```cpp
class SystemMarkManager
{
public:
  SystemMarkManager();

  StaticMarkPoint & SelectionMark() { return *m_selectionMark; }
  MyPositionMarkPoint & MyPositionMark() { return *m_myPositionMark; }

  // UserMark CRUD for non-bookmark marks
  template <typename UserMarkT>
  UserMarkT * CreateUserMark(m2::PointD const & ptOrg);

  template <typename UserMarkT>
  UserMarkT * GetMarkForEdit(kml::MarkId markId);

  void DeleteUserMark(kml::MarkId markId);

  UserMark const * GetUserMark(kml::MarkId markId) const;
  kml::MarkIdSet const & GetUserMarkIds(UserMark::Type type) const;

private:
  UserMarkLayers m_userMarkLayers;
  MarksCollection m_userMarks;
  StaticMarkPoint * m_selectionMark = nullptr;
  MyPositionMarkPoint * m_myPositionMark = nullptr;
};
```

BookmarkManager loses `m_userMarkLayers`, `m_userMarks`, `m_selectionMark`, `m_myPositionMark`, and all template `CreateUserMark`/`GetMarkForEdit`/`DeleteUserMarks` methods for non-bookmark types.

### Key Files
- `libs/map/system_mark_manager.hpp` (new)
- `libs/map/system_mark_manager.cpp` (new)
- `libs/map/bookmark_manager.hpp` - Remove system mark members and methods
- `libs/map/bookmark_manager.cpp` - Remove system mark code
- `libs/map/framework.hpp/cpp` - Own both managers, wire change tracking

### Tests
- `system_mark_manager_test.cpp` - CRUD for system marks
- Existing bookmark tests should not use system marks (verify)

### Risk
- Medium: `MarksChangesTracker` currently tracks both bookmark and system mark changes. Need to either split the tracker or have both managers feed into a shared tracker.
- `FindNearestUserMark()` searches across all mark types - needs access to both managers.

### Dependency
- Benefits from A (split components) being done first

---

## G. Async Data Access for Android

### Problem
All Android `nativeGet*` JNI methods block the main thread:
- `nativeGetBookmarkCategories()` - synchronous category listing
- `nativeGetBookmarkInfo()` - synchronous bookmark data fetch
- `nativeGetTrack()` - synchronous track data fetch
- Android Auto has 50-item limit workaround due to IPC serialization overhead
- No pagination support for large collections

### Proposed Solution

Add batch async query APIs:

```cpp
// C++ side
struct CategorySummary {
  kml::MarkGroupId id;
  std::string name;
  size_t bookmarkCount;
  size_t trackCount;
  bool visible;
};

void GetCategoriesSummaryAsync(
  std::function<void(std::vector<CategorySummary>)> callback);

void GetBookmarksPageAsync(
  kml::MarkGroupId groupId,
  size_t offset, size_t count,
  std::function<void(std::vector<BookmarkDTO>)> callback);
```

```java
// Java side
@WorkerThread
public void getCategoriesAsync(Consumer<List<BookmarkCategory>> callback);

@WorkerThread
public void getBookmarksPage(long groupId, int offset, int count,
                             Consumer<List<BookmarkInfo>> callback);
```

### Key Files
- `libs/map/bookmark_manager.hpp/cpp` - Add async query methods
- `android/sdk/src/main/cpp/.../BookmarkManager.cpp` - Add JNI for async queries
- `android/sdk/src/main/java/.../BookmarkManager.java` - Add Java async methods

### Tests
- C++ test: verify async callback receives correct data
- Android instrumentation test: verify data arrives on correct thread

### Risk
- Low: additive change, doesn't break existing sync API
- Can be done incrementally per query type

### Dependency
- Independent, can be done anytime. Benefits from E (unified bridge).

---

## H. Dead Code Cleanup

### Problem
Several unused artifacts remain from removed features (likely cloud sync):

1. **`ExpiredCategory`** struct (header line 798-804) + `m_expiredCategories` member (line 805): **completely unused** in .cpp
2. **`GetSeacrhAPIFn`** typo (header line 73): should be `GetSearchAPIFn`. The member `m_getSearchAPI` is declared but **never dereferenced** in bookmark_manager.cpp, only cast to nullptr in tests
3. **Compilation system** (`m_compilations`, `AttachCompilation`, `DetachCompilation`, `InferVisibility`): present but may be underused - needs verification before removal

### Confirmed Safe to Remove
- `ExpiredCategory` struct and `m_expiredCategories` vector
- Fix `GetSeacrhAPIFn` -> `GetSearchAPIFn` typo

### Confirmed Still Used (DO NOT remove)
- `RestoringCache` / `m_restoringCache` - used in `CreateCategories()` (line 2754) to restore access rules during category loading
- `m_lastCategoryUrl` - persists last edited category to settings (lines 1922, 1928, 2312, 2331)

### Key Files
- `libs/map/bookmark_manager.hpp` - Remove ExpiredCategory, fix typo
- `libs/map/bookmark_manager.cpp` - Remove any references
- `libs/map/map_tests/bookmarks_test.cpp` - Update nullptr cast for renamed type

### Tests
- All existing tests must pass
- Compile check is sufficient

### Risk
- Very low: removing unused code, fixing typo

---

## Implementation Priority & Dependencies

```
Phase 1 (Quick wins, independent):
  H. Dead Code Cleanup
  D. Simplify Edit Session

Phase 2 (Medium effort, foundational):
  A. Split into Focused Components (start with BookmarkSorter extraction)
  B. Unify Change Notification

Phase 3 (Depends on Phase 2):
  C. Extract Drape Communication (after B)
  F. Separate System Marks (after A)

Phase 4 (Independent, high effort):
  E. Unified Platform Bridge
  G. Async Data Access for Android
```
