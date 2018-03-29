#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/Framework.hpp"
#include "com/mapswithme/maps/UserMarkHelper.hpp"

#include "coding/zip_creator.hpp"
#include "map/place_page_info.hpp"

#include <utility>

using namespace std::placeholders;

namespace
{
::Framework * frm() { return g_framework->NativeFramework(); }

jclass g_bookmarkManagerClass;
jfieldID g_bookmarkManagerInstanceField;
jmethodID g_onBookmarksLoadingStartedMethod;
jmethodID g_onBookmarksLoadingFinishedMethod;
jmethodID g_onBookmarksFileLoadedMethod;
jmethodID g_onFinishKmlConversionMethod;
jmethodID g_onPreparedFileForSharingMethod;
jmethodID g_onSynchronizationStartedMethod;
jmethodID g_onSynchronizationFinishedMethod;
jmethodID g_onRestoreRequestedMethod;
jmethodID g_onRestoredFilesPreparedMethod;

void PrepareClassRefs(JNIEnv * env)
{
  if (g_bookmarkManagerClass)
    return;

  g_bookmarkManagerClass =
    jni::GetGlobalClassRef(env, "com/mapswithme/maps/bookmarks/data/BookmarkManager");
  g_bookmarkManagerInstanceField = jni::GetStaticFieldID(env, g_bookmarkManagerClass, "INSTANCE",
    "Lcom/mapswithme/maps/bookmarks/data/BookmarkManager;");

  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  g_onBookmarksLoadingStartedMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onBookmarksLoadingStarted", "()V");
  g_onBookmarksLoadingFinishedMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onBookmarksLoadingFinished", "()V");
  g_onBookmarksFileLoadedMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onBookmarksFileLoaded",
                     "(ZLjava/lang/String;Z)V");
  g_onFinishKmlConversionMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onFinishKmlConversion", "(Z)V");
  g_onPreparedFileForSharingMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onPreparedFileForSharing",
                     "(Lcom/mapswithme/maps/bookmarks/data/BookmarkSharingResult;)V");
  g_onSynchronizationStartedMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onSynchronizationStarted", "(I)V");
  g_onSynchronizationFinishedMethod =
    jni::GetMethodID(env, bookmarkManagerInstance,
                     "onSynchronizationFinished", "(IILjava/lang/String;)V");
  g_onRestoreRequestedMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onRestoreRequested", "(IJ)V");
  g_onRestoredFilesPreparedMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onRestoredFilesPrepared", "()V");
}

void OnAsyncLoadingStarted(JNIEnv * env)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onBookmarksLoadingStartedMethod);
  jni::HandleJavaException(env);
}

void OnAsyncLoadingFinished(JNIEnv * env)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onBookmarksLoadingFinishedMethod);
  jni::HandleJavaException(env);
}

void OnAsyncLoadingFileSuccess(JNIEnv * env, std::string const & fileName, bool isTemporaryFile)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  jni::TScopedLocalRef jFileName(env, jni::ToJavaString(env, fileName));
  env->CallVoidMethod(bookmarkManagerInstance, g_onBookmarksFileLoadedMethod,
                      true /* success */, jFileName.get(), isTemporaryFile);
  jni::HandleJavaException(env);
}

void OnAsyncLoadingFileError(JNIEnv * env, std::string const & fileName, bool isTemporaryFile)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  jni::TScopedLocalRef jFileName(env, jni::ToJavaString(env, fileName));
  env->CallVoidMethod(bookmarkManagerInstance, g_onBookmarksFileLoadedMethod,
                      false /* success */, jFileName.get(), isTemporaryFile);
  jni::HandleJavaException(env);
}

void OnFinishKmlConversion(JNIEnv * env, bool success)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onFinishKmlConversionMethod, success);
  jni::HandleJavaException(env);
}

void OnPreparedFileForSharing(JNIEnv * env, BookmarkManager::SharingResult const & result)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);

  static jclass const classBookmarkSharingResult = jni::GetGlobalClassRef(env,
    "com/mapswithme/maps/bookmarks/data/BookmarkSharingResult");
  // Java signature : BookmarkSharingResult(long categoryId, @Code int code,
  //                                        @NonNull String sharingPath,
  //                                        @NonNull String errorString)
  static jmethodID const ctorBookmarkSharingResult = jni::GetConstructorID(env,
    classBookmarkSharingResult, "(JILjava/lang/String;Ljava/lang/String;)V");

  jni::TScopedLocalRef const sharingPath(env, jni::ToJavaString(env, result.m_sharingPath));
  jni::TScopedLocalRef const errorString(env, jni::ToJavaString(env, result.m_errorString));
  jni::TScopedLocalRef const sharingResult(env, env->NewObject(classBookmarkSharingResult,
    ctorBookmarkSharingResult, static_cast<jlong>(result.m_categoryId),
    static_cast<jint>(result.m_code), sharingPath.get(), errorString.get()));

  env->CallVoidMethod(bookmarkManagerInstance, g_onPreparedFileForSharingMethod,
                      sharingResult.get());
  jni::HandleJavaException(env);
}

void OnSynchronizationStarted(JNIEnv * env, Cloud::SynchronizationType type)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onSynchronizationStartedMethod,
                      static_cast<jint>(type));
  jni::HandleJavaException(env);
}

void OnSynchronizationFinished(JNIEnv * env, Cloud::SynchronizationType type,
                               Cloud::SynchronizationResult result,
                               std::string const & errorStr)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onSynchronizationFinishedMethod,
                      static_cast<jint>(type), static_cast<jint>(result),
                      jni::ToJavaString(env, errorStr));
  jni::HandleJavaException(env);
}

void OnRestoreRequested(JNIEnv * env, Cloud::RestoringRequestResult result,
                        uint64_t backupTimestampInMs)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onRestoreRequestedMethod,
                      static_cast<jint>(result), static_cast<jlong>(backupTimestampInMs));
  jni::HandleJavaException(env);
}

void OnRestoredFilesPrepared(JNIEnv * env)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onRestoredFilesPreparedMethod);
  jni::HandleJavaException(env);
}
}  // namespace

extern "C"
{
using namespace jni;

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeShowBookmarkOnMap(
    JNIEnv * env, jobject thiz, jlong bmkId)
{
  frm()->ShowBookmark(static_cast<df::MarkID>(bmkId));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeLoadBookmarks(JNIEnv * env, jobject)
{
  PrepareClassRefs(env);
  BookmarkManager::AsyncLoadingCallbacks callbacks;
  callbacks.m_onStarted = std::bind(&OnAsyncLoadingStarted, env);
  callbacks.m_onFinished = std::bind(&OnAsyncLoadingFinished, env);
  callbacks.m_onFileSuccess = std::bind(&OnAsyncLoadingFileSuccess, env, _1, _2);
  callbacks.m_onFileError = std::bind(&OnAsyncLoadingFileError, env, _1, _2);
  frm()->GetBookmarkManager().SetAsyncLoadingCallbacks(std::move(callbacks));

  frm()->GetBookmarkManager().SetCloudHandlers(
    std::bind(&OnSynchronizationStarted, env, _1),
    std::bind(&OnSynchronizationFinished, env, _1, _2, _3),
    std::bind(&OnRestoreRequested, env, _1, _2),
    std::bind(&OnRestoredFilesPrepared, env));

  frm()->LoadBookmarks();
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetCategoriesCount(JNIEnv * env, jobject thiz)
{
  return frm()->GetBookmarkManager().GetBmGroupsIdList().size();
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetCategoryPositionById(
        JNIEnv * env, jobject thiz, jlong catId)
{
  auto & ids = frm()->GetBookmarkManager().GetBmGroupsIdList();
  jint position = 0;
  while (position < ids.size() && ids[position] != catId)
    ++position;
  return position;
}

JNIEXPORT jlong JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetCategoryIdByPosition(
        JNIEnv * env, jobject thiz, jint position)
{
  auto & ids = frm()->GetBookmarkManager().GetBmGroupsIdList();
  return static_cast<jlong>(position < ids.size() ? ids[position] : df::kInvalidMarkGroupId);
}

JNIEXPORT jlong JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeCreateCategory(
     JNIEnv * env, jobject thiz, jstring name)
{
  return static_cast<jlong>(frm()->GetBookmarkManager().CreateBookmarkCategory(ToNativeString(env, name)));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeDeleteCategory(
     JNIEnv * env, jobject thiz, jlong catId)
{
  auto const categoryId = static_cast<df::MarkGroupID>(catId);
  return static_cast<jboolean>(frm()->GetBookmarkManager().GetEditSession().DeleteBmCategory(categoryId));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeDeleteBookmark(JNIEnv *, jobject, jlong bmkId)
{
  frm()->GetBookmarkManager().GetEditSession().DeleteBookmark(static_cast<df::MarkID>(bmkId));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeDeleteTrack(
    JNIEnv * env, jobject thiz, jlong trkId)
{
  frm()->GetBookmarkManager().GetEditSession().DeleteTrack(static_cast<df::LineID>(trkId));
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeAddBookmarkToLastEditedCategory(
    JNIEnv * env, jobject thiz, jstring name, double lat, double lon)
{
  BookmarkManager & bmMng = frm()->GetBookmarkManager();

  m2::PointD const glbPoint(MercatorBounds::FromLatLon(lat, lon));
  BookmarkData bmkData(ToNativeString(env, name), frm()->LastEditedBMColor());
  auto const lastEditedCategory = frm()->LastEditedBMCategory();

  auto const * createdBookmark = bmMng.GetEditSession().CreateBookmark(glbPoint, bmkData, lastEditedCategory);

  place_page::Info & info = g_framework->GetPlacePageInfo();
  frm()->FillBookmarkInfo(*createdBookmark, info);

  return usermark_helper::CreateMapObject(env, info);
}

JNIEXPORT jlong JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetLastEditedCategory(
      JNIEnv * env, jobject thiz)
{
  return static_cast<jlong>(frm()->LastEditedBMCategory());
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGenerateUniqueFileName(JNIEnv * env, jclass thiz, jstring jBaseName)
{
  std::string baseName = ToNativeString(env, jBaseName);
  std::string bookmarkFileName = BookmarkManager::GenerateUniqueFileName(GetPlatform().SettingsDir(), baseName);
  return ToJavaString(env, bookmarkFileName);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeLoadKmzFile(JNIEnv * env, jobject thiz,
                                                                          jstring path, jboolean isTemporaryFile)
{
  frm()->AddBookmarksFile(ToNativeString(env, path), isTemporaryFile);
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeFormatNewBookmarkName(JNIEnv * env, jclass)
{
  return ToJavaString(env, g_framework->GetPlacePageInfo().FormatNewBookmarkName());
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeIsAsyncBookmarksLoadingInProgress(JNIEnv * env, jclass)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().IsAsyncLoadingInProgress());
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeIsVisible(
    JNIEnv * env, jobject thiz, jlong catId)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().IsVisible(static_cast<df::MarkGroupID>(catId)));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeSetVisibility(
    JNIEnv * env, jobject thiz, jlong catId, jboolean isVisible)
{
  frm()->GetBookmarkManager().GetEditSession().SetIsVisible(static_cast<df::MarkGroupID>(catId), isVisible);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeSetCategoryName(
    JNIEnv * env, jobject thiz, jlong catId, jstring name)
{
  frm()->GetBookmarkManager().GetEditSession().SetCategoryName(static_cast<df::MarkGroupID>(catId),
                                                               jni::ToNativeString(env, name));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetCategoryName(
     JNIEnv * env, jobject thiz, jlong catId)
{
  return ToJavaString(env, frm()->GetBookmarkManager().GetCategoryName(static_cast<df::MarkGroupID>(catId)));
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetBookmarksCount(
     JNIEnv * env, jobject thiz, jlong catId)
{
  return frm()->GetBookmarkManager().GetUserMarkIds(static_cast<df::MarkGroupID>(catId)).size();
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetTracksCount(
     JNIEnv * env, jobject thiz, jlong catId)
{
  return frm()->GetBookmarkManager().GetTrackIds(static_cast<df::MarkGroupID>(catId)).size();
}

// TODO(AlexZ): Get rid of UserMarks completely in UI code.
// TODO(yunikkk): Refactor java code to get all necessary info without Bookmark wrapper, and without hierarchy.
// If bookmark information is needed in the BookmarkManager, it does not relate in any way to Place Page info
// and should be passed separately via simple name string and lat lon to calculate a distance.
JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetBookmark(
     JNIEnv * env, jobject thiz, jlong bmkId)
{
  auto const * mark = frm()->GetBookmarkManager().GetBookmark(static_cast<df::MarkID>(bmkId));
  place_page::Info info;
  frm()->FillBookmarkInfo(*mark, info);
  return usermark_helper::CreateMapObject(env, info);
}

JNIEXPORT jlong JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetBookmarkIdByPosition(
        JNIEnv * env, jobject thiz, jlong catId, jint positionInCategory)
{
  auto & ids = frm()->GetBookmarkManager().GetUserMarkIds(static_cast<df::MarkGroupID>(catId));
  if (positionInCategory >= ids.size())
    return static_cast<jlong>(df::kInvalidMarkId);
  auto it = ids.begin();
  std::advance(it, positionInCategory);
  return static_cast<jlong>(*it);
}

static uint32_t shift(uint32_t v, uint8_t bitCount) { return v << bitCount; }

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetTrack(
      JNIEnv * env, jobject thiz, jlong trackId, jclass trackClazz)
{
  // Track(long trackId, long categoryId, String name, String lengthString, int color)
  static jmethodID const cId = jni::GetConstructorID(env, trackClazz,
                                                     "(JJLjava/lang/String;Ljava/lang/String;I)V");
  auto const * nTrack = frm()->GetBookmarkManager().GetTrack(static_cast<df::LineID>(trackId));

  ASSERT(nTrack, ("Track must not be null with id:)", trackId));

  std::string formattedLength;
  measurement_utils::FormatDistance(nTrack->GetLengthMeters(), formattedLength);

  dp::Color nColor = nTrack->GetColor(0);

  jint androidColor = shift(nColor.GetAlpha(), 24) +
                      shift(nColor.GetRed(), 16) +
                      shift(nColor.GetGreen(), 8) +
                      nColor.GetBlue();

  return env->NewObject(trackClazz, cId,
                        trackId, static_cast<jlong>(nTrack->GetGroupId()), jni::ToJavaString(env, nTrack->GetName()),
                        jni::ToJavaString(env, formattedLength), androidColor);
}

JNIEXPORT jlong JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetTrackIdByPosition(
        JNIEnv * env, jobject thiz, jlong catId, jint positionInCategory)
{
  auto & ids = frm()->GetBookmarkManager().GetTrackIds(static_cast<df::MarkGroupID>(catId));
  if (positionInCategory >= ids.size())
    return static_cast<jlong>(df::kInvalidLineId);
  auto it = ids.begin();
  std::advance(it, positionInCategory);
  return static_cast<jlong>(*it);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeSetCloudEnabled(
        JNIEnv * env, jobject thiz, jboolean enabled)
{
  frm()->GetBookmarkManager().SetCloudEnabled(enabled);
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeIsCloudEnabled(
        JNIEnv * env, jobject thiz)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().IsCloudEnabled());
}

JNIEXPORT jlong JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetLastSynchronizationTimestampInMs(
        JNIEnv * env, jobject thiz)
{
  return static_cast<jlong>(frm()->GetBookmarkManager().GetLastSynchronizationTimestampInMs());
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeIsUsedCategoryName(
        JNIEnv * env, jobject thiz, jstring name)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().IsUsedCategoryName(
                               ToNativeString(env, name)));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeAreAllCategoriesVisible(
        JNIEnv * env, jobject thiz)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().AreAllCategoriesVisible());
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeAreAllCategoriesInvisible(
        JNIEnv * env, jobject thiz)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().AreAllCategoriesInvisible());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeSetAllCategoriesVisibility(
        JNIEnv * env, jobject thiz, jboolean visible)
{
  frm()->GetBookmarkManager().SetAllCategoriesVisibility(static_cast<bool>(visible));
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetKmlFilesCountForConversion(
        JNIEnv * env, jobject thiz)
{
  return static_cast<jint>(frm()->GetBookmarkManager().GetKmlFilesCountForConversion());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeConvertAllKmlFiles(
        JNIEnv * env, jobject thiz)
{
  frm()->GetBookmarkManager().ConvertAllKmlFiles([env](bool success)
  {
    OnFinishKmlConversion(env, success);
  });
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativePrepareFileForSharing(
        JNIEnv * env, jobject thiz, jlong catId)
{
  frm()->GetBookmarkManager().PrepareFileForSharing(static_cast<df::MarkGroupID>(catId),
    [env](BookmarkManager::SharingResult const & result)
  {
    OnPreparedFileForSharing(env, result);
  });
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeIsCategoryEmpty(
        JNIEnv * env, jobject thiz, jlong catId)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().IsCategoryEmpty(
    static_cast<df::MarkGroupID>(catId)));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeRequestRestoring(
        JNIEnv * env, jobject thiz)
{
  frm()->GetBookmarkManager().RequestCloudRestoring();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeApplyRestoring(
        JNIEnv * env, jobject thiz)
{
  frm()->GetBookmarkManager().ApplyCloudRestoring();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeCancelRestoring(
        JNIEnv * env, jobject thiz)
{
  frm()->GetBookmarkManager().CancelCloudRestoring();
}
}  // extern "C"
