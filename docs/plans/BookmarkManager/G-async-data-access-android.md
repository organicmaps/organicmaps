# G. Async Data Access for Android

## Problem

All Android `nativeGet*` JNI methods block the main thread:

```java
// BookmarkManager.java - all @MainThread
static native @Nullable Bookmark nativeUpdateBookmarkPlacePage(long bmId);
static native @NonNull Track nativeGetTrack(long trackId, @NonNull Class<Track> trackClazz);
static native BookmarkCategory[] nativeGetBookmarkCategories();
static native @Nullable BookmarkInfo nativeGetBookmarkInfo(long bookmarkId);
```

**Impact:**
- UI jank with large bookmark collections (1000+ bookmarks)
- Android Auto has 50-item limit workaround in `BookmarksLoader.java` (line 49) due to IPC serialization overhead
- No pagination -- entire dataset transferred at once
- Category listing requires full JNI round-trip per query
- `CacheBookmarkCategoriesDataProvider` exists as a workaround but has stale data issues

**Current workaround (double DataProvider)**:
```java
// BookmarkManager.java lines 35-38
private final BookmarkCategoriesDataProvider mCategoriesCoreDataProvider;
private BookmarkCategoriesDataProvider mCurrentDataProvider;
// Switches to cache after loading finishes (line 129)
```

This cache is only updated on `onBookmarksChanged()` (line 109), creating a window where UI shows stale data.

## Proposed Solution

### Phase 1: Paginated Queries (C++ side)

```cpp
// libs/map/bookmark_manager.hpp additions

struct BookmarkPage
{
  std::vector<BookmarkInfo> items;
  size_t totalCount = 0;
  size_t offset = 0;
};

struct TrackPage
{
  struct TrackSummary
  {
    kml::TrackId id;
    std::string name;
    double lengthMeters;
    uint32_t colorARGB;
  };
  std::vector<TrackSummary> items;
  size_t totalCount = 0;
  size_t offset = 0;
};

// Synchronous but lightweight (returns summaries, not full data)
BookmarkPage GetBookmarksPage(kml::MarkGroupId groupId, size_t offset, size_t count) const;
TrackPage GetTracksPage(kml::MarkGroupId groupId, size_t offset, size_t count) const;
```

### Phase 2: Async Queries (C++ side)

```cpp
// For expensive operations (e.g., with address resolution)
using BookmarkPageCallback = platform::SafeCallback<void(BookmarkPage)>;
using TrackPageCallback = platform::SafeCallback<void(TrackPage)>;

void GetBookmarksPageAsync(kml::MarkGroupId groupId, size_t offset, size_t count,
                           BookmarkPageCallback && callback);
void GetTracksPageAsync(kml::MarkGroupId groupId, size_t offset, size_t count,
                        TrackPageCallback && callback);
```

### Phase 3: Android JNI + Java Integration

```cpp
// BookmarkManager.cpp - new JNI functions

JNIEXPORT jobjectArray JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeGetBookmarksPage(
    JNIEnv * env, jclass, jlong groupId, jint offset, jint count)
{
  auto const & frm = g_framework->NativeFramework();
  auto const page = frm.GetBookmarkManager().GetBookmarksPage(
      static_cast<kml::MarkGroupId>(groupId),
      static_cast<size_t>(offset),
      static_cast<size_t>(count));

  // Convert to Java BookmarkInfo[]
  return ToJavaBookmarkInfoArray(env, page.items);
}

JNIEXPORT jint JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeGetBookmarksCount(
    JNIEnv * env, jclass, jlong groupId)
{
  auto const & frm = g_framework->NativeFramework();
  auto const * group = frm.GetBookmarkManager().GetGroup(static_cast<kml::MarkGroupId>(groupId));
  return static_cast<jint>(group->GetUserMarks().size());
}
```

```java
// BookmarkManager.java additions

public static final int DEFAULT_PAGE_SIZE = 50;

@MainThread
public List<BookmarkInfo> getBookmarksPage(long groupId, int offset, int count) {
    return Arrays.asList(nativeGetBookmarksPage(groupId, offset, count));
}

@MainThread
public int getBookmarksCount(long groupId) {
    return nativeGetBookmarksCount(groupId);
}

// For Android Auto (replaces current 50-item workaround)
@MainThread
public List<BookmarkInfo> getBookmarksPageForCar(long groupId, int offset) {
    return getBookmarksPage(groupId, offset, CAR_PAGE_SIZE);
}

private static native BookmarkInfo[] nativeGetBookmarksPage(long groupId, int offset, int count);
private static native int nativeGetBookmarksCount(long groupId);
```

### Phase 4: RecyclerView Integration

```java
// BookmarksPagedAdapter.java (new)
public class BookmarksPagedAdapter extends RecyclerView.Adapter<BookmarkViewHolder>
{
  private final long mGroupId;
  private final SparseArray<BookmarkInfo> mCache = new SparseArray<>();
  private int mTotalCount = -1;

  @Override
  public int getItemCount() {
    if (mTotalCount < 0)
      mTotalCount = BookmarkManager.INSTANCE.getBookmarksCount(mGroupId);
    return mTotalCount;
  }

  @Override
  public void onBindViewHolder(BookmarkViewHolder holder, int position) {
    BookmarkInfo info = mCache.get(position);
    if (info == null) {
      // Load page containing this position
      int pageStart = (position / PAGE_SIZE) * PAGE_SIZE;
      List<BookmarkInfo> page = BookmarkManager.INSTANCE.getBookmarksPage(
          mGroupId, pageStart, PAGE_SIZE);
      for (int i = 0; i < page.size(); i++)
        mCache.put(pageStart + i, page.get(i));
      info = mCache.get(position);
    }
    holder.bind(info);
  }

  public void invalidateCache() {
    mCache.clear();
    mTotalCount = -1;
    notifyDataSetChanged();
  }
}
```

## Files to Create

| File | Content |
|------|---------|
| (none in Phase 1-2, methods added to existing files) | |
| `android/app/src/main/java/.../BookmarksPagedAdapter.java` | Phase 4 only |

## Files to Modify

| File | Changes |
|------|---------|
| `libs/map/bookmark_manager.hpp` | Add `GetBookmarksPage()`, `GetTracksPage()` methods + structs |
| `libs/map/bookmark_manager.cpp` | Implement paginated queries (~50 lines) |
| `android/sdk/src/main/cpp/.../BookmarkManager.cpp` | Add 2-4 new JNI functions |
| `android/sdk/src/main/java/.../BookmarkManager.java` | Add Java page query methods |
| `android/libs/car/src/main/java/.../BookmarksLoader.java` | Replace 50-item workaround with pagination |

## Tests

### C++ tests
```cpp
// libs/map/map_tests/bookmarks_test.cpp additions

TEST(BookmarkManager, GetBookmarksPage)
{
  // Create 100 bookmarks
  auto catId = bmManager.CreateBookmarkCategory("Test");
  for (int i = 0; i < 100; ++i) {
    auto session = bmManager.GetEditSession();
    session.CreateBookmark(MakeBookmarkData("BM" + std::to_string(i)), catId);
  }

  // Page 1
  auto page1 = bmManager.GetBookmarksPage(catId, 0, 20);
  TEST_EQUAL(page1.items.size(), 20, ());
  TEST_EQUAL(page1.totalCount, 100, ());
  TEST_EQUAL(page1.offset, 0, ());

  // Page 2
  auto page2 = bmManager.GetBookmarksPage(catId, 20, 20);
  TEST_EQUAL(page2.items.size(), 20, ());
  TEST_EQUAL(page2.offset, 20, ());

  // Last page (partial)
  auto lastPage = bmManager.GetBookmarksPage(catId, 90, 20);
  TEST_EQUAL(lastPage.items.size(), 10, ());

  // Beyond range
  auto empty = bmManager.GetBookmarksPage(catId, 200, 20);
  TEST(empty.items.empty(), ());
  TEST_EQUAL(empty.totalCount, 100, ());
}

TEST(BookmarkManager, GetTracksPage)
{
  // Similar to above but for tracks
}

TEST(BookmarkManager, PageEmptyCategory)
{
  auto catId = bmManager.CreateBookmarkCategory("Empty");
  auto page = bmManager.GetBookmarksPage(catId, 0, 20);
  TEST(page.items.empty(), ());
  TEST_EQUAL(page.totalCount, 0, ());
}
```

### Android instrumentation tests
```java
@Test
public void testBookmarksPage() {
    long catId = BookmarkManager.INSTANCE.createCategory("Test");
    // Add 50 bookmarks via BookmarkManager
    // ...

    List<BookmarkInfo> page = BookmarkManager.INSTANCE.getBookmarksPage(catId, 0, 20);
    assertEquals(20, page.size());

    int count = BookmarkManager.INSTANCE.getBookmarksCount(catId);
    assertEquals(50, count);
}
```

## Risks & Mitigations

- **Low risk**: Additive API. Existing sync methods remain unchanged.
- **Consistency**: Between page queries, bookmarks may be added/deleted. Mitigation: `totalCount` returned with each page allows UI to detect changes and re-query.
- **Performance**: Paginated queries iterate `kml::MarkIdSet` which is a `std::set` (ordered). Skip to offset costs O(offset). Mitigation: for typical page sizes (20-50) and practical collection sizes (<10K), this is fast enough. For very large collections, consider caching sorted ID vectors.
- **JNI overhead**: Each page query crosses JNI boundary. Mitigation: page size of 50 is a good balance (Android Auto already uses this limit).

## Dependencies
- Independent of all other plans. Can be done anytime.
- Benefits from plan E (unified bridge) -- DTOs make JNI conversion simpler.

## Priority
Phase 4 (independent). Start with Phase 1-2 (C++ side, ~2 hours), then Phase 3 (JNI, ~2 hours).
