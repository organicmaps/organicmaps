#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/bookmarks/data/Bookmark.hpp"
#include "app/organicmaps/sdk/bookmarks/data/BookmarkCategory.hpp"
#include "app/organicmaps/sdk/bookmarks/data/MapObject.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/util/Distance.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/place_page_info.hpp"

#include "coding/zip_creator.hpp"

#include "platform/localization.hpp"
#include "platform/preferred_languages.hpp"

#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include <limits>
#include <utility>

using namespace jni;
using namespace std::placeholders;

namespace
{
jclass g_bookmarkManagerClass;
jfieldID g_bookmarkManagerInstanceField;
jmethodID g_onBookmarksChangedMethod;
jmethodID g_onBookmarksLoadingStartedMethod;
jmethodID g_onBookmarksLoadingFinishedMethod;
jmethodID g_onBookmarksFileLoadedMethod;
jmethodID g_onPreparedFileForSharingMethod;
jmethodID g_onElevationActivePointChangedMethod;
jmethodID g_onElevationCurrentPositionChangedMethod;

jclass g_sortedBlockClass;
jmethodID g_sortedBlockConstructor;
jclass g_longClass;
jmethodID g_longConstructor;
jmethodID g_onBookmarksSortingCompleted;
jmethodID g_onBookmarksSortingCancelled;
jmethodID g_bookmarkInfoConstructor;
jclass g_bookmarkInfoClass;

void PrepareClassRefs(JNIEnv * env)
{
  if (g_bookmarkManagerClass)
    return;

  g_bookmarkManagerClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/BookmarkManager");
  g_bookmarkManagerInstanceField = jni::GetStaticFieldID(env, g_bookmarkManagerClass, "INSTANCE",
                                                         "Lapp/organicmaps/sdk/bookmarks/data/BookmarkManager;");

  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass, g_bookmarkManagerInstanceField);
  g_onBookmarksChangedMethod = jni::GetMethodID(env, bookmarkManagerInstance, "onBookmarksChanged", "()V");
  g_onBookmarksLoadingStartedMethod =
      jni::GetMethodID(env, bookmarkManagerInstance, "onBookmarksLoadingStarted", "()V");
  g_onBookmarksLoadingFinishedMethod =
      jni::GetMethodID(env, bookmarkManagerInstance, "onBookmarksLoadingFinished", "()V");
  g_onBookmarksFileLoadedMethod =
      jni::GetMethodID(env, bookmarkManagerInstance, "onBookmarksFileLoaded", "(ZLjava/lang/String;Z)V");
  g_onPreparedFileForSharingMethod = jni::GetMethodID(env, bookmarkManagerInstance, "onPreparedFileForSharing",
                                                      "(Lapp/organicmaps/sdk/bookmarks/data/BookmarkSharingResult;)V");

  g_longClass = jni::GetGlobalClassRef(env, "java/lang/Long");
  g_longConstructor = jni::GetConstructorID(env, g_longClass, "(J)V");
  g_sortedBlockClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/SortedBlock");
  g_sortedBlockConstructor =
      jni::GetConstructorID(env, g_sortedBlockClass, "(Ljava/lang/String;[Ljava/lang/Long;[Ljava/lang/Long;)V");

  g_onBookmarksSortingCompleted = jni::GetMethodID(env, bookmarkManagerInstance, "onBookmarksSortingCompleted",
                                                   "([Lapp/organicmaps/sdk/bookmarks/data/SortedBlock;J)V");
  g_onBookmarksSortingCancelled = jni::GetMethodID(env, bookmarkManagerInstance, "onBookmarksSortingCancelled", "(J)V");
  g_bookmarkInfoClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/BookmarkInfo");
  g_bookmarkInfoConstructor = jni::GetConstructorID(env, g_bookmarkInfoClass, "(JJ)V");

  g_onElevationCurrentPositionChangedMethod =
      jni::GetMethodID(env, bookmarkManagerInstance, "onElevationCurrentPositionChanged", "()V");
  g_onElevationActivePointChangedMethod =
      jni::GetMethodID(env, bookmarkManagerInstance, "onElevationActivePointChanged", "()V");
}

void OnElevationCurPositionChanged(JNIEnv * env)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass, g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onElevationCurrentPositionChangedMethod);
  jni::HandleJavaException(env);
}

void OnElevationActivePointChanged(JNIEnv * env)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass, g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onElevationActivePointChangedMethod);
  jni::HandleJavaException(env);
}

void OnBookmarksChanged(JNIEnv * env)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass, g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onBookmarksChangedMethod);
  jni::HandleJavaException(env);
}

void OnAsyncLoadingStarted(JNIEnv * env)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass, g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onBookmarksLoadingStartedMethod);
  jni::HandleJavaException(env);
}

void OnAsyncLoadingFinished(JNIEnv * env)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass, g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onBookmarksLoadingFinishedMethod);
  jni::HandleJavaException(env);
}

void OnAsyncLoadingFileSuccess(JNIEnv * env, std::string const & fileName, bool isTemporaryFile)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass, g_bookmarkManagerInstanceField);
  jni::TScopedLocalRef jFileName(env, jni::ToJavaString(env, fileName));
  env->CallVoidMethod(bookmarkManagerInstance, g_onBookmarksFileLoadedMethod, true /* success */, jFileName.get(),
                      isTemporaryFile);
  jni::HandleJavaException(env);
}

void OnAsyncLoadingFileError(JNIEnv * env, std::string const & fileName, bool isTemporaryFile)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass, g_bookmarkManagerInstanceField);
  jni::TScopedLocalRef jFileName(env, jni::ToJavaString(env, fileName));
  env->CallVoidMethod(bookmarkManagerInstance, g_onBookmarksFileLoadedMethod, false /* success */, jFileName.get(),
                      isTemporaryFile);
  jni::HandleJavaException(env);
}

void OnPreparedFileForSharing(JNIEnv * env, BookmarkManager::SharingResult const & result)
{
  static jclass const classBookmarkSharingResult =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/BookmarkSharingResult");
  // BookmarkSharingResult(long[] categoriesIds, @Code int code, @NonNull String sharingPath, @NonNull String mimeType,
  // @NonNull String errorString)
  static jmethodID const ctorBookmarkSharingResult = jni::GetConstructorID(
      env, classBookmarkSharingResult, "([JILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

  static_assert(sizeof(jlong) == sizeof(decltype(result.m_categoriesIds)::value_type));
  jsize const categoriesIdsSize = static_cast<jsize>(result.m_categoriesIds.size());
  jni::ScopedLocalRef<jlongArray> categoriesIds(env, env->NewLongArray(categoriesIdsSize));
  env->SetLongArrayRegion(categoriesIds.get(), 0, categoriesIdsSize,
                          reinterpret_cast<jlong const *>(result.m_categoriesIds.data()));
  jni::TScopedLocalRef const sharingPath(env, jni::ToJavaString(env, result.m_sharingPath));
  jni::TScopedLocalRef const mimeType(env, jni::ToJavaString(env, result.m_mimeType));
  jni::TScopedLocalRef const errorString(env, jni::ToJavaString(env, result.m_errorString));

  jni::TScopedLocalRef const sharingResult(
      env, env->NewObject(classBookmarkSharingResult, ctorBookmarkSharingResult, categoriesIds.get(),
                          static_cast<jint>(result.m_code), sharingPath.get(), mimeType.get(), errorString.get()));

  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass, g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onPreparedFileForSharingMethod, sharingResult.get());
  jni::HandleJavaException(env);
}

void OnCategorySortingResults(JNIEnv * env, long long timestamp,
                              BookmarkManager::SortedBlocksCollection && sortedBlocks,
                              BookmarkManager::SortParams::Status status)
{
  ASSERT(g_bookmarkManagerClass, ());
  ASSERT(g_sortedBlockClass, ());
  ASSERT(g_sortedBlockConstructor, ());

  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass, g_bookmarkManagerInstanceField);

  if (status == BookmarkManager::SortParams::Status::Cancelled)
  {
    env->CallVoidMethod(bookmarkManagerInstance, g_onBookmarksSortingCancelled, static_cast<jlong>(timestamp));
    jni::HandleJavaException(env);
    return;
  }

  jni::TScopedLocalObjectArrayRef blocksRef(
      env, jni::ToJavaArray(env, g_sortedBlockClass, sortedBlocks,
                            [](JNIEnv * env, BookmarkManager::SortedBlock const & block)
  {
    jni::TScopedLocalRef blockNameRef(env, jni::ToJavaString(env, block.m_blockName));

    jni::TScopedLocalObjectArrayRef marksRef(
        env, jni::ToJavaArray(env, g_longClass, block.m_markIds, [](JNIEnv * env, kml::MarkId const & markId)
    { return env->NewObject(g_longClass, g_longConstructor, static_cast<jlong>(markId)); }));

    jni::TScopedLocalObjectArrayRef tracksRef(
        env, jni::ToJavaArray(env, g_longClass, block.m_trackIds, [](JNIEnv * env, kml::TrackId const & trackId)
    { return env->NewObject(g_longClass, g_longConstructor, static_cast<jlong>(trackId)); }));

    return env->NewObject(g_sortedBlockClass, g_sortedBlockConstructor, blockNameRef.get(), marksRef.get(),
                          tracksRef.get());
  }));
  env->CallVoidMethod(bookmarkManagerInstance, g_onBookmarksSortingCompleted, blocksRef.get(),
                      static_cast<jlong>(timestamp));
  jni::HandleJavaException(env);
}
}  // namespace

extern "C"
{
JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeShowBookmarkOnMap(JNIEnv *, jobject,
                                                                                               jlong bmkId)
{
  frm()->ShowBookmark(static_cast<kml::MarkId>(bmkId));
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeShowBookmarkCategoryOnMap(JNIEnv *, jobject, jlong catId)
{
  frm()->ShowBookmarkCategory(static_cast<kml::MarkGroupId>(catId), true /* animated */);
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeLoadBookmarks(JNIEnv * env, jclass)
{
  PrepareClassRefs(env);
  BookmarkManager::AsyncLoadingCallbacks callbacks;
  callbacks.m_onStarted = std::bind(&OnAsyncLoadingStarted, env);
  callbacks.m_onFinished = std::bind(&OnAsyncLoadingFinished, env);
  callbacks.m_onFileSuccess = std::bind(&OnAsyncLoadingFileSuccess, env, _1, _2);
  callbacks.m_onFileError = std::bind(&OnAsyncLoadingFileError, env, _1, _2);
  frm()->GetBookmarkManager().SetAsyncLoadingCallbacks(std::move(callbacks));

  frm()->GetBookmarkManager().SetBookmarksChangedCallback(std::bind(&OnBookmarksChanged, env));

  frm()->LoadBookmarks();
}

JNIEXPORT jlong Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeCreateCategory(JNIEnv * env, jobject,
                                                                                             jstring name)
{
  auto const categoryId = frm()->GetBookmarkManager().CreateBookmarkCategory(ToNativeString(env, name));
  frm()->GetBookmarkManager().SetLastEditedBmCategory(categoryId);
  return static_cast<jlong>(categoryId);
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeDeleteCategory(JNIEnv *, jobject,
                                                                                                jlong catId)
{
  auto const categoryId = static_cast<kml::MarkGroupId>(catId);
  // `permanently` should be set to false when the Recently Deleted Lists feature be implemented
  return static_cast<jboolean>(
      frm()->GetBookmarkManager().GetEditSession().DeleteBmCategory(categoryId, true /* permanently */));
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeDeleteBookmark(JNIEnv *, jobject,
                                                                                            jlong bmkId)
{
  frm()->GetBookmarkManager().GetEditSession().DeleteBookmark(static_cast<kml::MarkId>(bmkId));
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeDeleteTrack(JNIEnv *, jobject, jlong trkId)
{
  frm()->GetBookmarkManager().GetEditSession().DeleteTrack(static_cast<kml::TrackId>(trkId));
}

JNIEXPORT jobject Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeAddBookmarkToLastEditedCategory(
    JNIEnv * env, jobject, double lat, double lon)
{
  if (!frm()->HasPlacePageInfo())
    return nullptr;

  BookmarkManager & bmMng = frm()->GetBookmarkManager();

  place_page::Info const & info = g_framework->GetPlacePageInfo();

  kml::BookmarkData bmData;
  bmData.m_name = info.FormatNewBookmarkName();
  bmData.m_color.m_predefinedColor = frm()->LastEditedBMColor();
  bmData.m_point = mercator::FromLatLon(lat, lon);
  auto const lastEditedCategory = frm()->LastEditedBMCategory();

  if (info.IsFeature())
    SaveFeatureTypes(info.GetTypes(), bmData);

  auto const * createdBookmark = bmMng.GetEditSession().CreateBookmark(std::move(bmData), lastEditedCategory);

  auto buildInfo = info.GetBuildInfo();
  buildInfo.m_match = place_page::BuildInfo::Match::Everything;
  buildInfo.m_userMarkId = createdBookmark->GetId();
  frm()->UpdatePlacePageInfoForCurrentSelection(buildInfo);

  return CreateMapObject(env, g_framework->GetPlacePageInfo());
}

JNIEXPORT jlong Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeGetLastEditedCategory(JNIEnv *, jobject)
{
  return static_cast<jlong>(frm()->LastEditedBMCategory());
}

JNIEXPORT jint Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeGetLastEditedColor(JNIEnv *, jobject)
{
  return static_cast<jint>(kml::kColorIndexMap[E2I(frm()->LastEditedBMColor())]);
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeLoadBookmarksFile(JNIEnv * env, jclass,
                                                                                               jstring path,
                                                                                               jboolean isTemporaryFile)
{
  frm()->AddBookmarksFile(ToNativeString(env, path), isTemporaryFile);
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeIsAsyncBookmarksLoadingInProgress(JNIEnv *, jclass)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().IsAsyncLoadingInProgress());
}

JNIEXPORT jobject Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeUpdateBookmarkPlacePage(JNIEnv * env,
                                                                                                        jobject,
                                                                                                        jlong bmkId)
{
  if (!frm()->HasPlacePageInfo())
    return nullptr;

  auto & info = g_framework->GetPlacePageInfo();
  auto buildInfo = info.GetBuildInfo();
  buildInfo.m_userMarkId = static_cast<kml::MarkId>(bmkId);
  frm()->UpdatePlacePageInfoForCurrentSelection(buildInfo);

  return CreateMapObject(env, g_framework->GetPlacePageInfo());
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeUpdateTrackPlacePage(JNIEnv * env, jobject)
{
  if (!frm()->HasPlacePageInfo())
    return;

  frm()->UpdatePlacePageInfoForCurrentSelection();
}

JNIEXPORT jobject Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeGetBookmarkInfo(JNIEnv * env, jobject,
                                                                                                jlong bmkId)
{
  auto const bookmark = frm()->GetBookmarkManager().GetBookmark(static_cast<kml::MarkId>(bmkId));
  if (!bookmark)
    return nullptr;
  return env->NewObject(g_bookmarkInfoClass, g_bookmarkInfoConstructor, static_cast<jlong>(bookmark->GetGroupId()),
                        static_cast<jlong>(bmkId));
}

static uint32_t shift(uint32_t v, uint8_t bitCount)
{
  return v << bitCount;
}

JNIEXPORT jobject Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeGetTrack(JNIEnv * env, jobject,
                                                                                         jlong trackId,
                                                                                         jclass trackClazz)
{
  // Track(long trackId, long categoryId, String name, String lengthString, int color)
  static jmethodID const cId =
      jni::GetConstructorID(env, trackClazz, "(JJLjava/lang/String;Lapp/organicmaps/sdk/util/Distance;I)V");
  auto const * nTrack = frm()->GetBookmarkManager().GetTrack(static_cast<kml::TrackId>(trackId));

  ASSERT(nTrack, ("Track must not be null with id:)", trackId));

  return env->NewObject(trackClazz, cId, trackId, static_cast<jlong>(nTrack->GetGroupId()),
                        jni::ToJavaString(env, nTrack->GetName()),
                        ToJavaDistance(env, platform::Distance::CreateFormatted(nTrack->GetLengthMeters())),
                        nTrack->GetColor(0).GetARGB());
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeIsUsedCategoryName(JNIEnv * env, jclass, jstring name)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().IsUsedCategoryName(ToNativeString(env, name)));
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativePrepareForSearch(JNIEnv *, jclass,
                                                                                              jlong catId)
{
  frm()->GetBookmarkManager().PrepareForSearch(static_cast<kml::MarkGroupId>(catId));
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeAreAllCategoriesInvisible(JNIEnv *, jclass)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().AreAllCategoriesInvisible());
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeAreAllCategoriesVisible(JNIEnv *, jclass)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().AreAllCategoriesVisible());
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeSetAllCategoriesVisibility(
    JNIEnv *, jclass, jboolean visible)
{
  frm()->GetBookmarkManager().SetAllCategoriesVisibility(static_cast<bool>(visible));
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativePrepareTrackFileForSharing(
    JNIEnv * env, jclass, jlong trackId, jint kmlFileType)
{
  frm()->GetBookmarkManager().PrepareTrackFileForSharing(static_cast<kml::TrackId>(trackId),
                                                         [env](BookmarkManager::SharingResult const & result)
  { OnPreparedFileForSharing(env, result); }, static_cast<KmlFileType>(kmlFileType));
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativePrepareFileForSharing(JNIEnv * env, jclass,
                                                                                                   jlongArray catIds,
                                                                                                   jint kmlFileType)
{
  auto const size = env->GetArrayLength(catIds);
  kml::GroupIdCollection catIdsVector(size);
  static_assert(sizeof(jlong) == sizeof(decltype(catIdsVector)::value_type));
  env->GetLongArrayRegion(catIds, 0, size, reinterpret_cast<jlong *>(catIdsVector.data()));
  frm()->GetBookmarkManager().PrepareFileForSharing(std::move(catIdsVector),
                                                    [env](BookmarkManager::SharingResult const & result)
  { OnPreparedFileForSharing(env, result); }, static_cast<KmlFileType>(kmlFileType));
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeSetNotificationsEnabled(JNIEnv *, jclass,
                                                                                                     jboolean enabled)
{
  frm()->GetBookmarkManager().SetNotificationsEnabled(static_cast<bool>(enabled));
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeAreNotificationsEnabled(JNIEnv *, jclass)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().AreNotificationsEnabled());
}

JNIEXPORT jobject JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeGetBookmarkCategory(JNIEnv * env, jobject, jlong id)
{
  return ToJavaBookmarkCategory(env, static_cast<kml::MarkGroupId>(id));
}

JNIEXPORT jobjectArray JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeGetBookmarkCategories(JNIEnv * env, jobject)
{
  auto const & bm = frm()->GetBookmarkManager();
  auto const & ids = bm.GetSortedBmGroupIdList();

  return ToJavaBookmarkCategories(env, ids);
}

JNIEXPORT jint JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeGetBookmarkCategoriesCount(JNIEnv * env, jobject)
{
  auto const & bm = frm()->GetBookmarkManager();
  auto const count = bm.GetBmGroupsCount();

  return static_cast<jint>(count);
}

JNIEXPORT jobjectArray Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeGetChildrenCategories(
    JNIEnv * env, jobject, jlong parentId)
{
  auto const & bm = frm()->GetBookmarkManager();
  auto const ids = bm.GetChildrenCategories(static_cast<kml::MarkGroupId>(parentId));

  return ToJavaBookmarkCategories(env, ids);
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeGetSortedCategory(
    JNIEnv * env, jobject, jlong catId, jint sortingType, jboolean hasMyPosition, jdouble lat, jdouble lon,
    jlong timestamp)
{
  auto & bm = frm()->GetBookmarkManager();
  BookmarkManager::SortParams sortParams;
  sortParams.m_groupId = static_cast<kml::MarkGroupId>(catId);
  sortParams.m_sortingType = static_cast<BookmarkManager::SortingType>(sortingType);
  sortParams.m_hasMyPosition = static_cast<bool>(hasMyPosition);
  sortParams.m_myPosition = mercator::FromLatLon(static_cast<double>(lat), static_cast<double>(lon));
  sortParams.m_onResults = bind(&OnCategorySortingResults, env, timestamp, _1, _2);

  bm.GetSortedCategory(sortParams);
}

constexpr static uint8_t ExtractByte(uint32_t number, uint8_t byteIdx)
{
  return (number >> (8 * byteIdx)) & 0xFF;
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeSetElevationCurrentPositionChangedListener(JNIEnv * env,
                                                                                                         jclass)
{
  frm()->GetBookmarkManager().SetElevationMyPositionChangedCallback(std::bind(&OnElevationCurPositionChanged, env));
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeRemoveElevationCurrentPositionChangedListener(JNIEnv *,
                                                                                                            jclass)
{
  frm()->GetBookmarkManager().SetElevationMyPositionChangedCallback(nullptr);
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeSetElevationActivePoint(
    JNIEnv *, jclass, jlong trackId, jdouble distanceInMeters, jdouble latitude, jdouble longitude)
{
  auto & bm = frm()->GetBookmarkManager();
  bm.SetElevationActivePoint(static_cast<kml::TrackId>(trackId), {latitude, longitude},
                             static_cast<double>(distanceInMeters));
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeSetElevationActiveChangedListener(JNIEnv * env, jclass)
{
  frm()->GetBookmarkManager().SetElevationActivePointChangedCallback(std::bind(&OnElevationActivePointChanged, env));
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeRemoveElevationActiveChangedListener(JNIEnv *, jclass)
{
  frm()->GetBookmarkManager().SetElevationActivePointChangedCallback(nullptr);
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_sdk_widget_placepage_PlacePageButtonFactory_nativeHasRecentlyDeletedBookmark(JNIEnv *, jclass)
{
  return frm()->GetBookmarkManager().HasRecentlyDeletedBookmark();
}
}  // extern "C"
