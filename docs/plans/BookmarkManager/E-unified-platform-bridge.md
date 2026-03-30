# E. Unified Platform Bridge (C++ Side)

## Problem

iOS and Android maintain independent bridge layers with significant duplication:

### iOS (`iphone/CoreApi/CoreApi/Bookmarks/MWMBookmarksManager.mm`, ~907 lines)
- `NSHashTable<id<MWMBookmarksObserver>>` for weak observer set
- 4 enum conversion functions (color, sorting type x2, file type)
- `loopObservers:` pattern for safe iteration
- `__weak`/`__strong` self captures in 4 async callbacks
- Wrapper classes: `MWMBookmark`, `MWMTrack`, `MWMBookmarkGroup`
- 6 Swift view controllers manually register/unregister observers

### Android (`android/sdk/src/main/cpp/.../BookmarkManager.cpp`, ~557 lines + Java ~700 lines)
- `PrepareClassRefs()` caches 15+ JNI class/method references
- 8 callback dispatchers with JNI string conversions
- 30+ `JNIEXPORT` functions with type marshaling
- Java: 5 listener interfaces, double DataProvider switching
- `BookmarkCategoriesCache` with separate update mechanism

### Duplication
- Both implement: observer lifecycle, enum conversion, data wrapping, callback dispatch
- Every new feature requires parallel changes in 3 places (C++, iOS, Android)
- Inconsistent behavior: Android caches categories, iOS doesn't
- Different error handling: Android has null checks in JNI, iOS uses optional protocol methods

## Proposed Solution

Create a C++ `BookmarkBridge` that provides a platform-neutral API with data transfer objects:

### Data Transfer Objects

```cpp
// libs/map/bookmark_bridge.hpp

struct BookmarkDTO
{
  int64_t id = 0;
  int64_t categoryId = 0;
  std::string name;
  std::string description;
  double lat = 0.0;
  double lon = 0.0;
  int colorIndex = 0;
  std::string iconName;
  std::string featureType;
  std::string address;
  double scale = 0.0;
};

struct TrackDTO
{
  int64_t id = 0;
  int64_t categoryId = 0;
  std::string name;
  double lengthMeters = 0.0;
  uint32_t colorARGB = 0;
};

struct CategoryDTO
{
  int64_t id = 0;
  std::string name;
  std::string author;
  std::string description;
  std::string annotation;
  std::string imageUrl;
  std::string serverId;
  size_t bookmarkCount = 0;
  size_t trackCount = 0;
  bool visible = true;
  bool empty = true;
  int accessRules = 0;
  int type = 0;  // category/collection/day
};

struct SortedBlockDTO
{
  std::string blockName;
  std::vector<int64_t> bookmarkIds;
  std::vector<int64_t> trackIds;
};
```

### Observer Interface

```cpp
class BookmarkBridgeObserver
{
public:
  virtual ~BookmarkBridgeObserver() = default;

  virtual void OnBookmarksLoadingStarted() {}
  virtual void OnBookmarksLoadingFinished() {}
  virtual void OnBookmarksFileLoadSuccess(std::string const & filePath, bool isTemporary) {}
  virtual void OnBookmarksFileLoadError(std::string const & filePath, bool isTemporary) {}

  virtual void OnBookmarksChanged() {}

  virtual void OnBookmarkDeleted(int64_t bookmarkId) {}
  virtual void OnCategoryDeleted(int64_t categoryId) {}

  virtual void OnSortingCompleted(std::vector<SortedBlockDTO> && blocks, int64_t timestamp) {}
  virtual void OnSortingCancelled(int64_t timestamp) {}

  virtual void OnSharingResult(int code, std::string const & path,
                               std::string const & mimeType, std::string const & error) {}

  virtual void OnElevationActivePointChanged() {}
  virtual void OnElevationMyPositionChanged() {}
};
```

### Bridge Class

```cpp
class BookmarkBridge
{
public:
  explicit BookmarkBridge(BookmarkManager & bmManager);

  // Observer with RAII token
  class ObserverToken
  {
  public:
    ~ObserverToken();
    ObserverToken(ObserverToken &&) noexcept;
  private:
    friend class BookmarkBridge;
    BookmarkBridge * m_bridge;
    BookmarkBridgeObserver * m_observer;
  };

  ObserverToken AddObserver(BookmarkBridgeObserver & observer);

  // Categories
  std::vector<CategoryDTO> GetCategories() const;
  CategoryDTO GetCategory(int64_t categoryId) const;
  size_t GetCategoriesCount() const;
  int64_t CreateCategory(std::string const & name);
  bool DeleteCategory(int64_t id, bool permanently);
  void SetCategoryVisible(int64_t id, bool visible);
  void SetAllCategoriesVisible(bool visible);
  bool AreAllCategoriesVisible() const;
  bool AreAllCategoriesInvisible() const;
  bool IsUsedCategoryName(std::string const & name) const;

  // Bookmarks
  std::optional<BookmarkDTO> GetBookmarkInfo(int64_t id) const;
  void DeleteBookmark(int64_t id);
  void UpdateBookmark(int64_t id, int64_t groupId,
                      std::string const & title, int colorIndex,
                      std::string const & description);
  void ShowBookmarkOnMap(int64_t id);

  // Tracks
  std::optional<TrackDTO> GetTrackInfo(int64_t id) const;
  void DeleteTrack(int64_t id);
  void ShowBookmarkCategoryOnMap(int64_t categoryId);

  // Sorting
  std::vector<int> GetAvailableSortingTypes(int64_t categoryId, bool hasMyPosition) const;
  void GetSortedCategory(int64_t categoryId, int sortingType,
                         bool hasMyPosition, double lat, double lon,
                         int64_t timestamp);

  // File operations
  void LoadBookmarks();
  void LoadBookmarkFile(std::string const & filePath, bool isTemporary);
  bool IsAsyncLoadingInProgress() const;

  // Sharing
  void PrepareFileForSharing(std::vector<int64_t> const & categoryIds, int fileType);
  void PrepareTrackFileForSharing(int64_t trackId, int fileType);

  // Notifications
  void SetNotificationsEnabled(bool enabled);

  // Elevation
  void SetElevationActivePoint(int64_t trackId, double lat, double lon, double distance);

  // Recently deleted
  bool HasRecentlyDeletedBookmark() const;

private:
  void RemoveObserver(BookmarkBridgeObserver & observer);

  BookmarkManager & m_bmManager;
  std::vector<BookmarkBridgeObserver *> m_observers;

  // DTO conversion helpers
  static BookmarkDTO ToBookmarkDTO(Bookmark const & bm, std::string const & address);
  static TrackDTO ToTrackDTO(Track const & track);
  static CategoryDTO ToCategoryDTO(BookmarkManager const & bm, kml::MarkGroupId id);
};
```

### Platform Bridge Simplification

**iOS (after):**
```objc
// MWMBookmarksManager becomes a thin wrapper
@interface MWMBookmarksManager () <BookmarkBridgeObserverObjC>
{
  BookmarkBridge * _bridge;
  BookmarkBridge::ObserverToken _token;
}
@end

- (NSArray<MWMBookmarkGroup *> *)categories {
  auto dtos = _bridge->GetCategories();
  // Convert DTOs to ObjC objects
}
```

**Android JNI (after):**
```cpp
JNIEXPORT jobject JNICALL nativeGetBookmarkInfo(JNIEnv * env, jclass, jlong id) {
  auto const dto = GetBridge().GetBookmarkInfo(id);
  if (!dto) return nullptr;
  return CreateJavaBookmarkInfo(env, *dto);  // Simple DTO -> Java conversion
}
```

## Files to Create

| File | Content |
|------|---------|
| `libs/map/bookmark_bridge.hpp` | DTOs + BookmarkBridgeObserver + BookmarkBridge class |
| `libs/map/bookmark_bridge.cpp` | Implementation (~400 lines): conversion + delegation |

## Files to Modify (incremental migration)

| File | Changes |
|------|---------|
| `libs/map/CMakeLists.txt` | Add new files |
| `iphone/CoreApi/CoreApi/Bookmarks/MWMBookmarksManager.mm` | Migrate methods one at a time to use bridge |
| `android/sdk/src/main/cpp/.../BookmarkManager.cpp` | Migrate JNI functions to use bridge |
| `libs/map/framework.hpp/cpp` | Create and own BookmarkBridge |

## Tests

```cpp
// libs/map/map_tests/bookmark_bridge_test.cpp

// Test: DTO conversion accuracy
TEST(BookmarkBridge, BookmarkDTOConversion)
{
  // Create a bookmark via BookmarkManager
  auto session = bmManager.GetEditSession();
  auto * bm = session.CreateBookmark(MakeBookmarkData("Test", 10.0, 20.0), catId);

  // Get it via bridge
  auto dto = bridge.GetBookmarkInfo(bm->GetId());
  TEST(dto.has_value(), ());
  TEST_EQUAL(dto->name, "Test", ());
  TEST_ALMOST_EQUAL_ABS(dto->lat, 10.0, 1e-5, ());
  TEST_ALMOST_EQUAL_ABS(dto->lon, 20.0, 1e-5, ());
}

// Test: Observer lifecycle with RAII token
TEST(BookmarkBridge, ObserverToken)
{
  TestObserver observer;
  {
    auto token = bridge.AddObserver(observer);
    // Observer receives events
    bridge.LoadBookmarkFile("test.kml", true);
    TEST(observer.m_loadingStarted, ());
  }
  // Token destroyed, observer automatically removed
  observer.Reset();
  bridge.LoadBookmarkFile("test2.kml", true);
  TEST(!observer.m_loadingStarted, ());  // No callback received
}

// Test: Categories list matches BookmarkManager
TEST(BookmarkBridge, CategoriesList)
{
  auto categories = bridge.GetCategories();
  TEST_EQUAL(categories.size(), bmManager.GetBmGroupsCount(), ());
  for (auto const & cat : categories)
    TEST(bmManager.HasBmCategory(static_cast<kml::MarkGroupId>(cat.id)), ());
}
```

## Migration Strategy

1. Create `BookmarkBridge` with all DTOs and methods
2. Add bridge to Framework
3. Migrate one iOS method at a time (e.g., `categories` getter first)
4. Verify iOS tests pass after each method
5. Migrate one Android JNI function at a time
6. Verify Android tests pass
7. Once all methods migrated, remove redundant platform code

## Risks & Mitigations

- **Performance**: DTO copy overhead vs direct pointer access. Mitigation: DTOs are small value types; for large collections (1000+ bookmarks), consider returning IDs and lazy-loading details.
- **Incremental migration**: Both old and new paths coexist during migration. Mitigation: bridge delegates to BookmarkManager, so behavior is identical.
- **Observer thread safety**: Platform callbacks must run on main/GUI thread. Bridge dispatches on the thread where BookmarkManager callbacks fire (already GUI thread per ThreadChecker).

## Priority
Phase 4 (independent, high effort). Can start anytime. Each method migration is an independent unit of work.
