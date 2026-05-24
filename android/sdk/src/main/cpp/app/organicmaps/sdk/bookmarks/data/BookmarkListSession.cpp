#include "app/organicmaps/sdk/bookmarks/data/BookmarkListSession.hpp"
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/bookmarks/data/BookmarkInfo.hpp"
#include "app/organicmaps/sdk/bookmarks/data/Track.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "map/bookmark.hpp"
#include "map/bookmarks_search_params.hpp"
#include "map/search_api.hpp"

#include "geometry/mercator.hpp"

#include "kml/type_utils.hpp"

#include "base/assert.hpp"

#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
enum class SessionMode
{
  Default,
  Search,
  Sorted
};

enum class RowType
{
  Track = 0,
  Bookmark = 1,
  Section = 2,
  Description = 3
};

enum class SectionKind
{
  None = 0,
  Description = 1,
  Tracks = 2,
  Bookmarks = 3,
  Custom = 4
};

struct RowSpec
{
  RowType m_type;
  jlong m_stableId;
  SectionKind m_sectionKind = SectionKind::None;
  std::string m_title;
  std::string m_description;
  kml::MarkId m_bookmarkId = kml::kInvalidMarkId;
  kml::TrackId m_trackId = kml::kInvalidTrackId;
};

class BookmarkListSession
{
public:
  BookmarkListSession(JNIEnv * env, jobject javaSession, kml::MarkGroupId categoryId);
  ~BookmarkListSession();

  void ShowDefault();
  bool Search(std::string query, jlong handle);
  void Sort(BookmarkManager::SortingType sortingType, bool hasMyPosition, m2::PointD myPosition, jlong handle);
  void OnBookmarksChanged(jlong handle);

  void SetHandle(jlong handle) { m_handle = handle; }

  static BookmarkListSession * GetSession(jlong handle);

private:
  void OnSearchResults(uint64_t generation, search::BookmarksSearchParams::Results results,
                       search::BookmarksSearchParams::Status status, jlong handle);
  void OnSortedResults(uint64_t generation, BookmarkManager::SortedBlocksCollection && sortedBlocks,
                       BookmarkManager::SortParams::Status status);
  void CancelSearchIfNeeded();
  bool IsCategoryAvailable() const;

  std::vector<RowSpec> BuildDefaultRows() const;
  std::vector<RowSpec> BuildBookmarkRows(std::vector<kml::MarkId> const & bookmarkIds) const;
  std::vector<RowSpec> BuildSortedRows(BookmarkManager::SortedBlocksCollection const & sortedBlocks) const;
  void NotifySnapshot(bool loading, std::vector<RowSpec> rows);
  void NotifyLoading(bool loading);

  static jobject CreateRow(JNIEnv * env, RowSpec const & row);
  static RowSpec MakeBuiltInSection(jlong stableId, SectionKind sectionKind);
  static RowSpec MakeCustomSection(jlong stableId, std::string const & title);
  static RowSpec MakeDescriptionRow(jlong stableId, std::string title, std::string description);
  static RowSpec MakeBookmarkRow(kml::MarkId bookmarkId);
  static RowSpec MakeTrackRow(kml::TrackId trackId);
  static std::string GetDescriptionText(kml::CategoryData const & categoryData);

  jobject m_javaSession;
  kml::MarkGroupId m_categoryId;
  jlong m_handle = 0;
  SessionMode m_mode = SessionMode::Default;
  uint64_t m_generation = 0;
  std::string m_query;
  BookmarkManager::SortingType m_sortingType = BookmarkManager::SortingType::ByType;
  bool m_hasMyPosition = false;
  m2::PointD m_myPosition = {0.0, 0.0};
  std::vector<RowSpec> m_rows;

  // nativeGetRow needs access to m_rows and CreateRow.
  friend jobject GetRowByIndex(JNIEnv * env, jlong handle, jint index);
};

jclass g_bookmarkListRowClass = nullptr;
jmethodID g_bookmarkListRowConstructor = nullptr;
jmethodID g_onSnapshotChangedMethod = nullptr;
jmethodID g_onLoadingChangedMethod = nullptr;

jlong g_nextSessionHandle = 1;
jlong g_activeSearchHandle = 0;
std::unordered_map<jlong, std::unique_ptr<BookmarkListSession>> g_sessions;

void PrepareClassRefs(JNIEnv * env, jobject javaSession)
{
  if (g_bookmarkListRowClass != nullptr)
    return;

  jclass const sessionClass = env->GetObjectClass(javaSession);
  g_onSnapshotChangedMethod = env->GetMethodID(sessionClass, "onSnapshotChanged", "(Z[I[J[I)V");
  g_onLoadingChangedMethod = env->GetMethodID(sessionClass, "onLoadingChanged", "(Z)V");
  env->DeleteLocalRef(sessionClass);

  g_bookmarkListRowClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/BookmarkListRow");
  g_bookmarkListRowConstructor = jni::GetConstructorID(env, g_bookmarkListRowClass,
                                                       "("
                                                       "I"                   // rowType
                                                       "J"                   // stableId
                                                       "I"                   // sectionKind
                                                       "Ljava/lang/String;"  // title
                                                       "Ljava/lang/String;"  // description
                                                       "Lapp/organicmaps/sdk/bookmarks/data/BookmarkInfo;"  // bookmark
                                                       "Lapp/organicmaps/sdk/bookmarks/data/Track;"         // track
                                                       ")V");

  jni::HandleJavaException(env);
}

BookmarkListSession::BookmarkListSession(JNIEnv * env, jobject javaSession, kml::MarkGroupId categoryId)
  : m_javaSession(env->NewGlobalRef(javaSession))
  , m_categoryId(categoryId)
{}

BookmarkListSession::~BookmarkListSession()
{
  if (m_javaSession == nullptr)
    return;

  if (JNIEnv * env = jni::GetEnv())
    env->DeleteGlobalRef(m_javaSession);
}

void BookmarkListSession::ShowDefault()
{
  CancelSearchIfNeeded();
  ++m_generation;
  m_mode = SessionMode::Default;
  NotifySnapshot(false /* loading */, BuildDefaultRows());
}

bool BookmarkListSession::Search(std::string query, jlong handle)
{
  ++m_generation;
  m_mode = SessionMode::Search;
  m_query = std::move(query);
  auto const generation = m_generation;

  if (!IsCategoryAvailable())
  {
    NotifySnapshot(false /* loading */, {});
    return false;
  }

  frm()->GetBookmarkManager().PrepareForSearch(m_categoryId);

  auto & searchApi = g_framework->NativeFramework()->GetSearchAPI();
  if (g_activeSearchHandle != 0 && g_activeSearchHandle != handle)
    searchApi.CancelSearch(search::Mode::Bookmarks);
  g_activeSearchHandle = handle;

  NotifyLoading(true);

  search::BookmarksSearchParams params{
      m_query, m_categoryId,
      [handle, generation](search::BookmarksSearchParams::Results results, search::BookmarksSearchParams::Status status)
  {
    if (auto * session = GetSession(handle))
      session->OnSearchResults(generation, std::move(results), status, handle);
  }};

  bool const started = searchApi.SearchInBookmarks(std::move(params));
  if (!started)
  {
    if (g_activeSearchHandle == handle)
      g_activeSearchHandle = 0;

    NotifySnapshot(false /* loading */, {});
  }
  return started;
}

void BookmarkListSession::Sort(BookmarkManager::SortingType sortingType, bool hasMyPosition, m2::PointD myPosition,
                               jlong handle)
{
  CancelSearchIfNeeded();
  ++m_generation;
  m_mode = SessionMode::Sorted;
  m_sortingType = sortingType;
  m_hasMyPosition = hasMyPosition;
  m_myPosition = myPosition;
  auto const generation = m_generation;

  if (!IsCategoryAvailable())
  {
    NotifySnapshot(false /* loading */, {});
    return;
  }

  if (!m_hasMyPosition && m_sortingType == BookmarkManager::SortingType::ByDistance)
  {
    NotifySnapshot(false /* loading */, BuildDefaultRows());
    return;
  }

  NotifyLoading(true);

  BookmarkManager::SortParams sortParams;
  sortParams.m_groupId = m_categoryId;
  sortParams.m_sortingType = m_sortingType;
  sortParams.m_hasMyPosition = m_hasMyPosition;
  sortParams.m_myPosition = m_myPosition;
  sortParams.m_onResults = [handle, generation](BookmarkManager::SortedBlocksCollection && sortedBlocks,
                                                BookmarkManager::SortParams::Status status)
  {
    if (auto * session = GetSession(handle))
      session->OnSortedResults(generation, std::move(sortedBlocks), status);
  };
  frm()->GetBookmarkManager().GetSortedCategory(sortParams);
}

void BookmarkListSession::OnBookmarksChanged(jlong handle)
{
  if (!IsCategoryAvailable())
  {
    CancelSearchIfNeeded();
    ++m_generation;
    m_mode = SessionMode::Default;
    NotifySnapshot(false /* loading */, {});
    return;
  }

  switch (m_mode)
  {
  case SessionMode::Default: ShowDefault(); return;
  case SessionMode::Search:
    if (m_query.empty())
    {
      ShowDefault();
    }
    else
    {
      // Refresh m_rows to reflect current bookmarks (a bookmark may have been deleted)
      // before re-running the search. Otherwise getRow() may dereference a stale ID.
      NotifySnapshot(true /* loading */, BuildDefaultRows());
      Search(m_query, handle);
    }
    return;
  case SessionMode::Sorted:
    // Same rationale as the Search branch above.
    NotifySnapshot(true /* loading */, BuildDefaultRows());
    Sort(m_sortingType, m_hasMyPosition, m_myPosition, handle);
    return;
  }
}

BookmarkListSession * BookmarkListSession::GetSession(jlong handle)
{
  auto const it = g_sessions.find(handle);
  if (it == g_sessions.end())
    return nullptr;
  return it->second.get();
}

void BookmarkListSession::OnSearchResults(uint64_t generation, search::BookmarksSearchParams::Results results,
                                          search::BookmarksSearchParams::Status status, jlong handle)
{
  if (generation != m_generation || m_mode != SessionMode::Search)
    return;

  frm()->GetBookmarkManager().FilterInvalidBookmarks(results);

  NotifySnapshot(status == search::BookmarksSearchParams::Status::InProgress, BuildBookmarkRows(results));

  if (status != search::BookmarksSearchParams::Status::InProgress && g_activeSearchHandle == handle)
    g_activeSearchHandle = 0;
}

void BookmarkListSession::OnSortedResults(uint64_t generation, BookmarkManager::SortedBlocksCollection && sortedBlocks,
                                          BookmarkManager::SortParams::Status status)
{
  if (generation != m_generation || m_mode != SessionMode::Sorted)
    return;

  if (status == BookmarkManager::SortParams::Status::Cancelled)
  {
    NotifySnapshot(false /* loading */, BuildDefaultRows());
    return;
  }

  NotifySnapshot(false /* loading */, BuildSortedRows(sortedBlocks));
}

void BookmarkListSession::CancelSearchIfNeeded()
{
  if (g_activeSearchHandle == 0 || g_activeSearchHandle != m_handle)
    return;

  g_framework->NativeFramework()->GetSearchAPI().CancelSearch(search::Mode::Bookmarks);
  g_activeSearchHandle = 0;
}

bool BookmarkListSession::IsCategoryAvailable() const
{
  return frm()->GetBookmarkManager().HasBmCategory(m_categoryId);
}

std::vector<RowSpec> BookmarkListSession::BuildDefaultRows() const
{
  auto & bm = frm()->GetBookmarkManager();
  CHECK(bm.HasBmCategory(m_categoryId), (m_categoryId));

  auto const & tracks = bm.GetTrackIds(m_categoryId);
  auto const & bookmarks = bm.GetUserMarkIds(m_categoryId);

  std::vector<RowSpec> rows;
  // 2 for description section + row, 1 header + N items per non-empty group.
  rows.reserve(2 + (tracks.empty() ? 0 : 1 + tracks.size()) + (bookmarks.empty() ? 0 : 1 + bookmarks.size()));

  auto const & categoryData = bm.GetCategoryData(m_categoryId);
  auto const categoryName = bm.GetCategoryName(m_categoryId);

  rows.push_back(MakeBuiltInSection(std::numeric_limits<jlong>::min() + 1, SectionKind::Description));
  rows.push_back(
      MakeDescriptionRow(std::numeric_limits<jlong>::min() + 2, categoryName, GetDescriptionText(categoryData)));

  if (!tracks.empty())
  {
    rows.push_back(MakeBuiltInSection(std::numeric_limits<jlong>::min() + 3, SectionKind::Tracks));
    for (auto const trackId : tracks)
      rows.push_back(MakeTrackRow(trackId));
  }

  if (!bookmarks.empty())
  {
    rows.push_back(MakeBuiltInSection(std::numeric_limits<jlong>::min() + 4, SectionKind::Bookmarks));
    for (auto const bookmarkId : bookmarks)
      rows.push_back(MakeBookmarkRow(bookmarkId));
  }

  return rows;
}

std::vector<RowSpec> BookmarkListSession::BuildBookmarkRows(std::vector<kml::MarkId> const & bookmarkIds) const
{
  std::vector<RowSpec> rows;
  rows.reserve(bookmarkIds.size());
  for (auto const bookmarkId : bookmarkIds)
    rows.push_back(MakeBookmarkRow(bookmarkId));
  return rows;
}

std::vector<RowSpec> BookmarkListSession::BuildSortedRows(
    BookmarkManager::SortedBlocksCollection const & sortedBlocks) const
{
  auto & bm = frm()->GetBookmarkManager();
  CHECK(bm.HasBmCategory(m_categoryId), (m_categoryId));

  size_t estimatedSize = 2;  // optional description section + row
  for (auto const & block : sortedBlocks)
    estimatedSize += 1 + block.m_markIds.size() + block.m_trackIds.size();

  std::vector<RowSpec> rows;
  rows.reserve(estimatedSize);

  auto const & categoryData = bm.GetCategoryData(m_categoryId);
  if (!kml::GetDefaultStr(categoryData.m_annotation).empty() || !kml::GetDefaultStr(categoryData.m_description).empty())
  {
    rows.push_back(MakeBuiltInSection(std::numeric_limits<jlong>::min() + 1, SectionKind::Description));
    rows.push_back(MakeDescriptionRow(std::numeric_limits<jlong>::min() + 2, bm.GetCategoryName(m_categoryId),
                                      GetDescriptionText(categoryData)));
  }

  jlong syntheticId = std::numeric_limits<jlong>::min() + 100;
  for (auto const & block : sortedBlocks)
  {
    if (block.m_markIds.empty() && block.m_trackIds.empty())
      continue;

    rows.push_back(MakeCustomSection(syntheticId++, block.m_blockName));
    for (auto const bookmarkId : block.m_markIds)
      rows.push_back(MakeBookmarkRow(bookmarkId));
    for (auto const trackId : block.m_trackIds)
      rows.push_back(MakeTrackRow(trackId));
  }

  return rows;
}

void BookmarkListSession::NotifySnapshot(bool loading, std::vector<RowSpec> rows)
{
  m_rows = std::move(rows);

  JNIEnv * env = jni::GetEnv();
  ASSERT(env != nullptr, ());

  auto const size = static_cast<jsize>(m_rows.size());

  std::vector<jint> types(size);
  std::vector<jlong> stableIds(size);
  std::vector<jint> sectionKinds(size);
  for (jsize i = 0; i < size; ++i)
  {
    types[i] = static_cast<jint>(m_rows[i].m_type);
    stableIds[i] = m_rows[i].m_stableId;
    sectionKinds[i] = static_cast<jint>(m_rows[i].m_sectionKind);
  }

  jni::ScopedLocalRef<jintArray> jTypes(env, env->NewIntArray(size));
  env->SetIntArrayRegion(jTypes.get(), 0, size, types.data());
  jni::ScopedLocalRef<jlongArray> jStableIds(env, env->NewLongArray(size));
  env->SetLongArrayRegion(jStableIds.get(), 0, size, stableIds.data());
  jni::ScopedLocalRef<jintArray> jSectionKinds(env, env->NewIntArray(size));
  env->SetIntArrayRegion(jSectionKinds.get(), 0, size, sectionKinds.data());

  env->CallVoidMethod(m_javaSession, g_onSnapshotChangedMethod, static_cast<jboolean>(loading), jTypes.get(),
                      jStableIds.get(), jSectionKinds.get());
  jni::HandleJavaException(env);
}

void BookmarkListSession::NotifyLoading(bool loading)
{
  JNIEnv * env = jni::GetEnv();
  ASSERT(env != nullptr, ());
  env->CallVoidMethod(m_javaSession, g_onLoadingChangedMethod, static_cast<jboolean>(loading));
  jni::HandleJavaException(env);
}

jobject BookmarkListSession::CreateRow(JNIEnv * env, RowSpec const & row)
{
  switch (row.m_type)
  {
  case RowType::Section:
  {
    jni::ScopedLocalRef<jstring> title(env, row.m_title.empty() ? nullptr : jni::ToJavaString(env, row.m_title));
    return env->NewObject(g_bookmarkListRowClass, g_bookmarkListRowConstructor, static_cast<jint>(RowType::Section),
                          row.m_stableId, static_cast<jint>(row.m_sectionKind), title.get(), nullptr, nullptr, nullptr);
  }
  case RowType::Description:
  {
    jni::ScopedLocalRef<jstring> title(env, jni::ToJavaString(env, row.m_title));
    jni::ScopedLocalRef<jstring> description(env, jni::ToJavaString(env, row.m_description));
    return env->NewObject(g_bookmarkListRowClass, g_bookmarkListRowConstructor, static_cast<jint>(RowType::Description),
                          row.m_stableId, static_cast<jint>(row.m_sectionKind), title.get(), description.get(), nullptr,
                          nullptr);
  }
  case RowType::Bookmark:
  {
    auto const * bookmark = frm()->GetBookmarkManager().GetBookmark(row.m_bookmarkId);
    ASSERT(bookmark, ("Bookmark not found:", row.m_bookmarkId));

    jni::ScopedLocalRef<jobject> bookmarkInfo(env, CreateBookmarkInfo(env, *bookmark));
    return env->NewObject(g_bookmarkListRowClass, g_bookmarkListRowConstructor, static_cast<jint>(RowType::Bookmark),
                          row.m_stableId, static_cast<jint>(row.m_sectionKind), nullptr, nullptr, bookmarkInfo.get(),
                          nullptr);
  }
  case RowType::Track:
  {
    auto const * track = frm()->GetBookmarkManager().GetTrack(row.m_trackId);
    ASSERT(track, ("Track not found:", row.m_trackId));

    jni::ScopedLocalRef<jobject> trackObject(env, CreateTrack(env, *track));
    return env->NewObject(g_bookmarkListRowClass, g_bookmarkListRowConstructor, static_cast<jint>(RowType::Track),
                          row.m_stableId, static_cast<jint>(row.m_sectionKind), nullptr, nullptr, nullptr,
                          trackObject.get());
  }
  }

  UNREACHABLE();
}

RowSpec BookmarkListSession::MakeBuiltInSection(jlong stableId, SectionKind sectionKind)
{
  RowSpec row;
  row.m_type = RowType::Section;
  row.m_stableId = stableId;
  row.m_sectionKind = sectionKind;
  return row;
}

RowSpec BookmarkListSession::MakeCustomSection(jlong stableId, std::string const & title)
{
  RowSpec row = MakeBuiltInSection(stableId, SectionKind::Custom);
  row.m_title = title;
  return row;
}

RowSpec BookmarkListSession::MakeDescriptionRow(jlong stableId, std::string title, std::string description)
{
  RowSpec row;
  row.m_type = RowType::Description;
  row.m_stableId = stableId;
  row.m_title = std::move(title);
  row.m_description = std::move(description);
  return row;
}

RowSpec BookmarkListSession::MakeBookmarkRow(kml::MarkId bookmarkId)
{
  RowSpec row;
  row.m_type = RowType::Bookmark;
  row.m_stableId = static_cast<jlong>(bookmarkId);
  row.m_bookmarkId = bookmarkId;
  return row;
}

RowSpec BookmarkListSession::MakeTrackRow(kml::TrackId trackId)
{
  RowSpec row;
  row.m_type = RowType::Track;
  row.m_stableId = -static_cast<jlong>(trackId) - 1;
  row.m_trackId = trackId;
  return row;
}

std::string BookmarkListSession::GetDescriptionText(kml::CategoryData const & categoryData)
{
  auto annotation = kml::GetDefaultStr(categoryData.m_annotation);
  if (!annotation.empty())
    return annotation;
  return kml::GetDefaultStr(categoryData.m_description);
}

jobject GetRowByIndex(JNIEnv * env, jlong handle, jint index)
{
  auto * session = BookmarkListSession::GetSession(handle);
  CHECK(session != nullptr, ("Session not found:", handle));
  CHECK(index >= 0 && index < static_cast<jint>(session->m_rows.size()),
        ("Index out of bounds:", index, session->m_rows.size()));
  return BookmarkListSession::CreateRow(env, session->m_rows[index]);
}
}  // namespace

void OnBookmarkListSessionsChanged(JNIEnv * env)
{
  (void)env;

  for (auto const & [handle, session] : g_sessions)
    session->OnBookmarksChanged(handle);
}

extern "C"
{
JNIEXPORT jlong JNICALL Java_app_organicmaps_sdk_bookmarks_data_BookmarkListSession_nativeCreate(JNIEnv * env, jclass,
                                                                                                 jobject session,
                                                                                                 jlong categoryId)
{
  PrepareClassRefs(env, session);

  auto nativeSession = std::make_unique<BookmarkListSession>(env, session, static_cast<kml::MarkGroupId>(categoryId));
  auto const handle = g_nextSessionHandle++;
  nativeSession->SetHandle(handle);
  g_sessions.emplace(handle, std::move(nativeSession));
  return handle;
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_bookmarks_data_BookmarkListSession_nativeDestroy(JNIEnv *, jclass,
                                                                                                 jlong nativePtr)
{
  auto const handle = nativePtr;
  if (g_activeSearchHandle == handle)
  {
    g_framework->NativeFramework()->GetSearchAPI().CancelSearch(search::Mode::Bookmarks);
    g_activeSearchHandle = 0;
  }

  g_sessions.erase(handle);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_bookmarks_data_BookmarkListSession_nativeShowDefault(JNIEnv *, jclass,
                                                                                                     jlong nativePtr)
{
  if (auto * session = BookmarkListSession::GetSession(nativePtr))
    session->ShowDefault();
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_bookmarks_data_BookmarkListSession_nativeSearch(JNIEnv * env,
                                                                                                    jclass,
                                                                                                    jlong nativePtr,
                                                                                                    jstring query)
{
  if (auto * session = BookmarkListSession::GetSession(nativePtr))
    return static_cast<jboolean>(session->Search(jni::ToNativeString(env, query), nativePtr));

  return static_cast<jboolean>(false);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_bookmarks_data_BookmarkListSession_nativeSort(
    JNIEnv *, jclass, jlong nativePtr, jint sortingType, jboolean hasMyPosition, jdouble lat, jdouble lon)
{
  if (auto * session = BookmarkListSession::GetSession(nativePtr))
  {
    session->Sort(static_cast<BookmarkManager::SortingType>(sortingType), static_cast<bool>(hasMyPosition),
                  mercator::FromLatLon(static_cast<double>(lat), static_cast<double>(lon)), nativePtr);
  }
}

JNIEXPORT jobject JNICALL Java_app_organicmaps_sdk_bookmarks_data_BookmarkListSession_nativeGetRow(JNIEnv * env, jclass,
                                                                                                   jlong nativePtr,
                                                                                                   jint index)
{
  return GetRowByIndex(env, nativePtr, index);
}
}  // extern "C"
