#include "app/organicmaps/sdk/bookmarks/data/BookmarkListSession.hpp"
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/util/Distance.hpp"

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
  void NotifySnapshot(bool loading, std::vector<RowSpec> const * rows) const;

  static jobjectArray BuildRowsArray(JNIEnv * env, std::vector<RowSpec> const & rows);
  static jobject CreateRow(JNIEnv * env, RowSpec const & row);
  static jobject CreateBookmarkInfo(JNIEnv * env, Bookmark const & bookmark);
  static jobject CreateTrack(JNIEnv * env, Track const & track);
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
};

jclass g_bookmarkListRowClass = nullptr;
jmethodID g_bookmarkListRowConstructor = nullptr;
jmethodID g_onSnapshotChangedMethod = nullptr;
jclass g_bookmarkListBookmarkInfoClass = nullptr;
jmethodID g_bookmarkListBookmarkInfoConstructor = nullptr;
jclass g_bookmarkListTrackClass = nullptr;
jmethodID g_bookmarkListTrackConstructor = nullptr;

jlong g_nextSessionHandle = 1;
jlong g_activeSearchHandle = 0;
std::unordered_map<jlong, std::unique_ptr<BookmarkListSession>> g_sessions;

void PrepareClassRefs(JNIEnv * env, jobject javaSession)
{
  if (g_bookmarkListRowClass != nullptr)
    return;

  jclass const sessionClass = env->GetObjectClass(javaSession);
  g_onSnapshotChangedMethod =
      env->GetMethodID(sessionClass, "onSnapshotChanged", "(Z[Lapp/organicmaps/sdk/bookmarks/data/BookmarkListRow;)V");
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

  g_bookmarkListBookmarkInfoClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/BookmarkInfo");
  g_bookmarkListBookmarkInfoConstructor =
      jni::GetConstructorID(env, g_bookmarkListBookmarkInfoClass,
                            "("
                            "J"                                                      // categoryId
                            "J"                                                      // bookmarkId
                            "Ljava/lang/String;"                                     // title
                            "Ljava/lang/String;"                                     // description
                            "Ljava/lang/String;"                                     // featureType
                            "I"                                                      // color
                            "I"                                                      // iconType
                            "Lapp/organicmaps/sdk/bookmarks/data/ParcelablePointD;"  // coords
                            "D"                                                      // scale
                            "Ljava/lang/String;"                                     // address
                            ")V");

  g_bookmarkListTrackClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/Track");
  g_bookmarkListTrackConstructor = jni::GetConstructorID(env, g_bookmarkListTrackClass,
                                                         "("
                                                         "J"                                    // id
                                                         "J"                                    // categoryId
                                                         "Ljava/lang/String;"                   // title
                                                         "Lapp/organicmaps/sdk/util/Distance;"  // length
                                                         "I"                                    // color
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
  auto rows = BuildDefaultRows();
  NotifySnapshot(false /* loading */, &rows);
}

bool BookmarkListSession::Search(std::string query, jlong handle)
{
  ++m_generation;
  m_mode = SessionMode::Search;
  m_query = std::move(query);
  auto const generation = m_generation;

  if (!IsCategoryAvailable())
  {
    std::vector<RowSpec> rows;
    NotifySnapshot(false /* loading */, &rows);
    return false;
  }

  frm()->GetBookmarkManager().PrepareForSearch(m_categoryId);

  auto & searchApi = g_framework->NativeFramework()->GetSearchAPI();
  if (g_activeSearchHandle != 0 && g_activeSearchHandle != handle)
    searchApi.CancelSearch(search::Mode::Bookmarks);
  g_activeSearchHandle = handle;

  NotifySnapshot(true /* loading */, nullptr);

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

    std::vector<RowSpec> rows;
    NotifySnapshot(false /* loading */, &rows);
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
    std::vector<RowSpec> rows;
    NotifySnapshot(false /* loading */, &rows);
    return;
  }

  if (!m_hasMyPosition && m_sortingType == BookmarkManager::SortingType::ByDistance)
  {
    auto rows = BuildDefaultRows();
    NotifySnapshot(false /* loading */, &rows);
    return;
  }

  NotifySnapshot(true /* loading */, nullptr);

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
    std::vector<RowSpec> rows;
    NotifySnapshot(false /* loading */, &rows);
    return;
  }

  switch (m_mode)
  {
  case SessionMode::Default: ShowDefault(); return;
  case SessionMode::Search:
    if (m_query.empty())
      ShowDefault();
    else
      Search(m_query, handle);
    return;
  case SessionMode::Sorted: Sort(m_sortingType, m_hasMyPosition, m_myPosition, handle); return;
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

  auto rows = BuildBookmarkRows(results);
  NotifySnapshot(status == search::BookmarksSearchParams::Status::InProgress, &rows);

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
    auto rows = BuildDefaultRows();
    NotifySnapshot(false /* loading */, &rows);
    return;
  }

  auto rows = BuildSortedRows(sortedBlocks);
  NotifySnapshot(false /* loading */, &rows);
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
  std::vector<RowSpec> rows;
  auto & bm = frm()->GetBookmarkManager();
  if (!bm.HasBmCategory(m_categoryId))
    return rows;

  auto const & categoryData = bm.GetCategoryData(m_categoryId);
  auto const categoryName = bm.GetCategoryName(m_categoryId);

  rows.push_back(MakeBuiltInSection(std::numeric_limits<jlong>::min() + 1, SectionKind::Description));
  rows.push_back(
      MakeDescriptionRow(std::numeric_limits<jlong>::min() + 2, categoryName, GetDescriptionText(categoryData)));

  auto const & tracks = bm.GetTrackIds(m_categoryId);
  if (!tracks.empty())
  {
    rows.push_back(MakeBuiltInSection(std::numeric_limits<jlong>::min() + 3, SectionKind::Tracks));
    for (auto const trackId : tracks)
      rows.push_back(MakeTrackRow(trackId));
  }

  auto const & bookmarks = bm.GetUserMarkIds(m_categoryId);
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
  std::vector<RowSpec> rows;
  auto & bm = frm()->GetBookmarkManager();
  if (!bm.HasBmCategory(m_categoryId))
    return rows;

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

void BookmarkListSession::NotifySnapshot(bool loading, std::vector<RowSpec> const * rows) const
{
  JNIEnv * env = jni::GetEnv();
  ASSERT(env != nullptr, ());

  jni::ScopedLocalRef<jobjectArray> rowsArray(env, rows == nullptr ? nullptr : BuildRowsArray(env, *rows));
  env->CallVoidMethod(m_javaSession, g_onSnapshotChangedMethod, static_cast<jboolean>(loading), rowsArray.get());
  jni::HandleJavaException(env);
}

jobjectArray BookmarkListSession::BuildRowsArray(JNIEnv * env, std::vector<RowSpec> const & rows)
{
  auto const size = static_cast<jsize>(rows.size());
  jobjectArray const array = env->NewObjectArray(size, g_bookmarkListRowClass, nullptr);
  for (jsize i = 0; i < size; ++i)
  {
    // PushLocalFrame/PopLocalFrame to free intermediate JNI refs (strings, ParcelablePointD, etc.)
    // created inside CreateRow/CreateBookmarkInfo/CreateTrack on each iteration.
    env->PushLocalFrame(16);
    jobject const rowObject = CreateRow(env, rows[i]);
    env->SetObjectArrayElement(array, i, rowObject);
    env->PopLocalFrame(nullptr);
  }
  return array;
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
    ASSERT(bookmark != nullptr, ("Bookmark not found:", row.m_bookmarkId));

    jni::ScopedLocalRef<jobject> bookmarkInfo(env, CreateBookmarkInfo(env, *bookmark));
    return env->NewObject(g_bookmarkListRowClass, g_bookmarkListRowConstructor, static_cast<jint>(RowType::Bookmark),
                          row.m_stableId, static_cast<jint>(row.m_sectionKind), nullptr, nullptr, bookmarkInfo.get(),
                          nullptr);
  }
  case RowType::Track:
  {
    auto const * track = frm()->GetBookmarkManager().GetTrack(row.m_trackId);
    ASSERT(track != nullptr, ("Track not found:", row.m_trackId));

    jni::ScopedLocalRef<jobject> trackObject(env, CreateTrack(env, *track));
    return env->NewObject(g_bookmarkListRowClass, g_bookmarkListRowConstructor, static_cast<jint>(RowType::Track),
                          row.m_stableId, static_cast<jint>(row.m_sectionKind), nullptr, nullptr, nullptr,
                          trackObject.get());
  }
  }

  UNREACHABLE();
}

jobject BookmarkListSession::CreateBookmarkInfo(JNIEnv * env, Bookmark const & bookmark)
{
  auto const title = jni::ToJavaString(env, bookmark.GetPreferredName());
  auto const description = jni::ToJavaString(env, bookmark.GetDescription());
  auto const featureType = jni::ToJavaString(env, kml::GetLocalizedFeatureType(bookmark.GetData().m_featureTypes));
  auto const color = static_cast<jint>(kml::kColorIndexMap[base::E2I(bookmark.GetColor())]);
  auto const iconType = static_cast<jint>(bookmark.GetData().m_icon);
  auto const coords = jni::GetNewParcelablePointD(env, bookmark.GetPivot());
  auto const scale = static_cast<jdouble>(bookmark.GetScale());
  auto const address = jni::ToJavaString(env, frm()->GetAddressAtPoint(bookmark.GetPivot()).FormatAddress());

  return env->NewObject(g_bookmarkListBookmarkInfoClass, g_bookmarkListBookmarkInfoConstructor,
                        static_cast<jlong>(bookmark.GetGroupId()), static_cast<jlong>(bookmark.GetId()), title,
                        description, featureType, color, iconType, coords, scale, address);
}

jobject BookmarkListSession::CreateTrack(JNIEnv * env, Track const & track)
{
  return env->NewObject(g_bookmarkListTrackClass, g_bookmarkListTrackConstructor, static_cast<jlong>(track.GetId()),
                        static_cast<jlong>(track.GetGroupId()), jni::ToJavaString(env, track.GetName()),
                        ToJavaDistance(env, platform::Distance::CreateFormatted(track.GetLengthMeters())),
                        track.GetColor(0).GetARGB());
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
  auto const annotation = kml::GetDefaultStr(categoryData.m_annotation);
  if (!annotation.empty())
    return annotation;
  return kml::GetDefaultStr(categoryData.m_description);
}
}  // namespace

void OnBookmarkListSessionsChanged(JNIEnv * env)
{
  (void)env;

  std::vector<jlong> handles;
  handles.reserve(g_sessions.size());
  for (auto const & [handle, session] : g_sessions)
    handles.push_back(handle);

  for (auto const handle : handles)
  {
    auto const it = g_sessions.find(handle);
    if (it == g_sessions.end())
      continue;

    it->second->OnBookmarksChanged(handle);
  }
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
}  // extern "C"
