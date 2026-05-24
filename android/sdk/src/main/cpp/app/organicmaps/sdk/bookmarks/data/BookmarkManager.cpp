#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/bookmarks/data/Bookmark.hpp"
#include "app/organicmaps/sdk/bookmarks/data/BookmarkCategory.hpp"
#include "app/organicmaps/sdk/bookmarks/data/BookmarkInfo.hpp"
#include "app/organicmaps/sdk/bookmarks/data/BookmarkListSession.hpp"
#include "app/organicmaps/sdk/bookmarks/data/MapObject.hpp"
#include "app/organicmaps/sdk/bookmarks/data/Track.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "kml/type_utils.hpp"

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
  OnBookmarkListSessionsChanged(env);
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
  bmData.m_point = mercator::FromLatLon(lat, lon);
  auto const lastEditedCategory = frm()->LastEditedBMCategory();
  bmData.m_color.m_predefinedColor = frm()->LastEditedBMColor();

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
  auto const * bookmark = frm()->GetBookmarkManager().GetBookmark(static_cast<kml::MarkId>(bmkId));
  if (!bookmark)
    return nullptr;

  return CreateBookmarkInfo(env, *bookmark);
}

static uint32_t shift(uint32_t v, uint8_t bitCount)
{
  return v << bitCount;
}

JNIEXPORT jobject Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativeGetTrack(JNIEnv * env, jobject,
                                                                                         jlong trackId,
                                                                                         jclass /* trackClazz */)
{
  auto const * track = frm()->GetBookmarkManager().GetTrack(static_cast<kml::TrackId>(trackId));
  ASSERT(track, ("Track must not be null with id:", trackId));

  return CreateTrack(env, *track);
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

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativePrepareTrackFileForSharing(JNIEnv * env,
                                                                                                        jclass,
                                                                                                        jlong trackId,
                                                                                                        jint fileType)
{
  frm()->GetBookmarkManager().PrepareTrackFileForSharing(static_cast<kml::TrackId>(trackId),
                                                         [env](BookmarkManager::SharingResult const & result)
  { OnPreparedFileForSharing(env, result); }, static_cast<FileType>(fileType));
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_BookmarkManager_nativePrepareFileForSharing(JNIEnv * env, jclass,
                                                                                                   jlongArray catIds,
                                                                                                   jint fileType)
{
  auto const size = env->GetArrayLength(catIds);
  kml::GroupIdCollection catIdsVector(size);
  static_assert(sizeof(jlong) == sizeof(decltype(catIdsVector)::value_type));
  env->GetLongArrayRegion(catIds, 0, size, reinterpret_cast<jlong *>(catIdsVector.data()));
  frm()->GetBookmarkManager().PrepareFileForSharing(std::move(catIdsVector),
                                                    [env](BookmarkManager::SharingResult const & result)
  { OnPreparedFileForSharing(env, result); }, static_cast<FileType>(fileType));
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
    JNIEnv *, jclass, jlong trackId, jdouble distanceInMeters)
{
  auto & bm = frm()->GetBookmarkManager();
  bm.SetElevationActivePoint(static_cast<kml::TrackId>(trackId), static_cast<double>(distanceInMeters));
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
