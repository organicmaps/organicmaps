#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/Framework.hpp"
#include "com/mapswithme/maps/UserMarkHelper.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/place_page_info.hpp"

#include "coding/zip_creator.hpp"

#include "platform/preferred_languages.hpp"

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
jmethodID g_onImportStartedMethod;
jmethodID g_onImportFinishedMethod;
jmethodID g_onTagsReceivedMethod;
jmethodID g_onCustomPropertiesReceivedMethod;
jmethodID g_onUploadStartedMethod;
jmethodID g_onUploadFinishedMethod;
jclass g_bookmarkCategoryClass;
jmethodID g_bookmarkCategoryConstructor;
jclass g_catalogTagClass;
jmethodID g_catalogTagConstructor;
jclass g_catalogTagsGroupClass;
jmethodID g_catalogTagsGroupConstructor;

jclass g_catalogCustomPropertyOptionClass;
jmethodID g_catalogCustomPropertyOptionConstructor;
jclass g_catalogCustomPropertyClass;
jmethodID g_catalogCustomPropertyConstructor;

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
    jni::GetMethodID(env, bookmarkManagerInstance, "onRestoreRequested", "(ILjava/lang/String;J)V");
  g_onRestoredFilesPreparedMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onRestoredFilesPrepared", "()V");
  g_onImportStartedMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onImportStarted", "(Ljava/lang/String;)V");
  g_onImportFinishedMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onImportFinished", "(Ljava/lang/String;JZ)V");
  g_onTagsReceivedMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onTagsReceived",
                     "(Z[Lcom/mapswithme/maps/bookmarks/data/CatalogTagsGroup;)V");
  g_onCustomPropertiesReceivedMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onCustomPropertiesReceived",
                     "(Z[Lcom/mapswithme/maps/bookmarks/data/CatalogCustomProperty;)V");

  g_onUploadStartedMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onUploadStarted", "(J)V");
  g_onUploadFinishedMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onUploadFinished", "(ILjava/lang/String;JJ)V");

  g_bookmarkCategoryClass =
    jni::GetGlobalClassRef(env, "com/mapswithme/maps/bookmarks/data/BookmarkCategory");
//public BookmarkCategory(long id,
//                          String name,
//                          String authorId,
//                          String authorName,
//                          String annotation,
//                          String desc,
//                          int tracksCount,
//                          int bookmarksCount,
//                          boolean fromCatalog,
//                          boolean isMyCategory,
//                          boolean isVisible)
  g_bookmarkCategoryConstructor =
      jni::GetConstructorID(env, g_bookmarkCategoryClass,
                            "(JLjava/lang/String;Ljava/lang/String;"
                            "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IIZZZI)V");

  g_catalogTagClass =
    jni::GetGlobalClassRef(env, "com/mapswithme/maps/bookmarks/data/CatalogTag");
//public CatalogTag(@NonNull String id, @NonNull String localizedName, float r, float g, float b)
  g_catalogTagConstructor =
    jni::GetConstructorID(env, g_catalogTagClass, "(Ljava/lang/String;Ljava/lang/String;FFF)V");

  g_catalogTagsGroupClass =
    jni::GetGlobalClassRef(env, "com/mapswithme/maps/bookmarks/data/CatalogTagsGroup");
//public CatalogTagsGroup(@NonNull String localizedName, @NonNull CatalogTag[] tags)
  g_catalogTagsGroupConstructor =
    jni::GetConstructorID(env, g_catalogTagsGroupClass,
                          "(Ljava/lang/String;[Lcom/mapswithme/maps/bookmarks/data/CatalogTag;)V");

  g_catalogCustomPropertyOptionClass =
    jni::GetGlobalClassRef(env, "com/mapswithme/maps/bookmarks/data/CatalogCustomPropertyOption");
//public CatalogCustomPropertyOption(@NonNull String value, @NonNull String localizedName)
  g_catalogCustomPropertyOptionConstructor =
    jni::GetConstructorID(env, g_catalogCustomPropertyOptionClass,
                          "(Ljava/lang/String;Ljava/lang/String;)V");
  g_catalogCustomPropertyClass =
    jni::GetGlobalClassRef(env, "com/mapswithme/maps/bookmarks/data/CatalogCustomProperty");
//public CatalogCustomProperty(@NonNull String key, @NonNull String localizedName,
//                             boolean isRequired, @NonNull CatalogCustomPropertyOption[] options)
  g_catalogCustomPropertyConstructor =
    jni::GetConstructorID(env, g_catalogCustomPropertyClass,
                          "(Ljava/lang/String;Ljava/lang/String;Z"
                          "[Lcom/mapswithme/maps/bookmarks/data/CatalogCustomPropertyOption;)V");
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
                        std::string const & deviceName, uint64_t backupTimestampInMs)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onRestoreRequestedMethod,
                      static_cast<jint>(result), jni::ToJavaString(env, deviceName),
                      static_cast<jlong>(backupTimestampInMs));
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

void OnImportStarted(JNIEnv * env, std::string const & serverId)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onImportStartedMethod,
                      jni::ToJavaString(env, serverId));
  jni::HandleJavaException(env);
}

void OnImportFinished(JNIEnv * env, std::string const & serverId, kml::MarkGroupId categoryId,
                      bool successful)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onImportFinishedMethod,
                      jni::ToJavaString(env, serverId), static_cast<jlong>(categoryId),
                      static_cast<jboolean>(successful));
  jni::HandleJavaException(env);
}

void OnTagsReceived(JNIEnv * env, bool successful, BookmarkCatalog::TagGroups const & groups)
{
  ASSERT(g_bookmarkManagerClass, ());
  ASSERT(g_catalogTagClass, ());
  ASSERT(g_catalogTagsGroupClass, ());

  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onTagsReceivedMethod,
                      static_cast<jboolean>(successful),
                      jni::ToJavaArray(env, g_catalogTagsGroupClass, groups,
                      [](JNIEnv * env, BookmarkCatalog::TagGroup const & tagGroup)
  {
    jni::TScopedLocalRef tagGroupNameRef(env, jni::ToJavaString(env, tagGroup.m_name));
    return env->NewObject(g_catalogTagsGroupClass, g_catalogTagsGroupConstructor,
                          tagGroupNameRef.get(),
                          jni::ToJavaArray(env, g_catalogTagClass, tagGroup.m_tags,
                          [](JNIEnv * env, BookmarkCatalog::Tag const & tag)
    {
      jni::TScopedLocalRef tagIdRef(env, jni::ToJavaString(env, tag.m_id));
      jni::TScopedLocalRef tagNameRef(env, jni::ToJavaString(env, tag.m_name));
      return env->NewObject(g_catalogTagClass, g_catalogTagConstructor,
                            tagIdRef.get(), tagNameRef.get(),
                            static_cast<jfloat>(tag.m_color[0]),
                            static_cast<jfloat>(tag.m_color[1]),
                            static_cast<jfloat>(tag.m_color[2]));
    }));
  }));
  jni::HandleJavaException(env);
}

void OnCustomPropertiesReceived(JNIEnv * env, bool successful,
                                BookmarkCatalog::CustomProperties const & properties)
{
  ASSERT(g_bookmarkManagerClass, ());
  ASSERT(g_catalogCustomPropertyOptionClass, ());
  ASSERT(g_catalogCustomPropertyClass, ());

  jni::TScopedLocalObjectArrayRef propsRef(env,
    jni::ToJavaArray(env, g_catalogCustomPropertyClass, properties,
                     [](JNIEnv * env, BookmarkCatalog::CustomProperty const & customProperty)
  {
    jni::TScopedLocalRef nameRef(env, jni::ToJavaString(env, customProperty.m_name));
    jni::TScopedLocalRef keyRef(env, jni::ToJavaString(env, customProperty.m_key));
    jni::TScopedLocalObjectArrayRef optionsRef(env,
      jni::ToJavaArray(env, g_catalogCustomPropertyOptionClass, customProperty.m_options,
      [](JNIEnv * env, BookmarkCatalog::CustomProperty::Option const & option)
    {
      jni::TScopedLocalRef valueRef(env, jni::ToJavaString(env, option.m_value));
      jni::TScopedLocalRef nameRef(env, jni::ToJavaString(env, option.m_name));
      return env->NewObject(g_catalogCustomPropertyOptionClass,
                            g_catalogCustomPropertyOptionConstructor,
                            valueRef.get(), nameRef.get());
    }));
    return env->NewObject(g_catalogCustomPropertyClass,
                          g_catalogCustomPropertyConstructor,
                          keyRef.get(), nameRef.get(),
                          static_cast<jboolean>(customProperty.m_isRequired),
                          optionsRef.get());
  }));

  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onCustomPropertiesReceivedMethod,
                      static_cast<jboolean>(successful), propsRef.get());
  jni::HandleJavaException(env);
}

void OnUploadStarted(JNIEnv * env, kml::MarkGroupId originCategoryId)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onUploadStartedMethod,
                      static_cast<jlong>(originCategoryId));
  jni::HandleJavaException(env);
}

void OnUploadFinished(JNIEnv * env, BookmarkCatalog::UploadResult uploadResult,
                      std::string const & description, kml::MarkGroupId originCategoryId,
                      kml::MarkGroupId resultCategoryId)
{
  ASSERT(g_bookmarkManagerClass, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  jni::TScopedLocalRef const descriptionStr(env, jni::ToJavaString(env, description));
  env->CallVoidMethod(bookmarkManagerInstance, g_onUploadFinishedMethod,
                      static_cast<jint>(uploadResult), descriptionStr.get(),
                      static_cast<jlong>(originCategoryId), static_cast<jlong>(resultCategoryId));
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
  frm()->ShowBookmark(static_cast<kml::MarkId>(bmkId));
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
    std::bind(&OnRestoreRequested, env, _1, _2, _3),
    std::bind(&OnRestoredFilesPrepared, env));

  frm()->GetBookmarkManager().SetCatalogHandlers(nullptr, nullptr,
                                                 std::bind(&OnImportStarted, env, _1),
                                                 std::bind(&OnImportFinished, env, _1, _2, _3),
                                                 std::bind(&OnUploadStarted, env, _1),
                                                 std::bind(&OnUploadFinished, env, _1, _2, _3, _4));
  frm()->LoadBookmarks();
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetCategoriesCount(
        JNIEnv * env, jobject thiz)
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
  return static_cast<jlong>(position < ids.size() ? ids[position] : kml::kInvalidMarkGroupId);
}

JNIEXPORT jlong JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeCreateCategory(
     JNIEnv * env, jobject thiz, jstring name)
{
  auto const categoryId = frm()->GetBookmarkManager().CreateBookmarkCategory(ToNativeString(env, name));
  frm()->GetBookmarkManager().SetLastEditedBmCategory(categoryId);
  return static_cast<jlong>(categoryId);
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeDeleteCategory(
     JNIEnv * env, jobject thiz, jlong catId)
{
  auto const categoryId = static_cast<kml::MarkGroupId>(catId);
  return static_cast<jboolean>(frm()->GetBookmarkManager().GetEditSession().DeleteBmCategory(categoryId));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeDeleteBookmark(JNIEnv *, jobject, jlong bmkId)
{
  frm()->GetBookmarkManager().GetEditSession().DeleteBookmark(static_cast<kml::MarkId>(bmkId));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeDeleteTrack(
    JNIEnv * env, jobject thiz, jlong trkId)
{
  frm()->GetBookmarkManager().GetEditSession().DeleteTrack(static_cast<kml::TrackId>(trkId));
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeAddBookmarkToLastEditedCategory(
    JNIEnv * env, jobject thiz, double lat, double lon)
{
  BookmarkManager & bmMng = frm()->GetBookmarkManager();

  place_page::Info & info = g_framework->GetPlacePageInfo();

  kml::BookmarkData bmData;
  bmData.m_name = info.FormatNewBookmarkName();
  bmData.m_color.m_predefinedColor = frm()->LastEditedBMColor();
  bmData.m_point = MercatorBounds::FromLatLon(lat, lon);
  auto const lastEditedCategory = frm()->LastEditedBMCategory();

  if (info.IsFeature())
    SaveFeatureTypes(info.GetTypes(), bmData);

  auto const * createdBookmark = bmMng.GetEditSession().CreateBookmark(std::move(bmData), lastEditedCategory);

  frm()->FillBookmarkInfo(*createdBookmark, info);

  return usermark_helper::CreateMapObject(env, info);
}

JNIEXPORT jlong JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetLastEditedCategory(
      JNIEnv * env, jobject thiz)
{
  return static_cast<jlong>(frm()->LastEditedBMCategory());
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetLastEditedColor(
        JNIEnv * env, jobject thiz)
{
  return static_cast<jint>(frm()->LastEditedBMColor());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeLoadKmzFile(JNIEnv * env, jobject thiz,
                                                                          jstring path, jboolean isTemporaryFile)
{
  frm()->AddBookmarksFile(ToNativeString(env, path), isTemporaryFile);
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
  return static_cast<jboolean>(frm()->GetBookmarkManager().IsVisible(static_cast<kml::MarkGroupId>(catId)));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeSetVisibility(
    JNIEnv * env, jobject thiz, jlong catId, jboolean isVisible)
{
  frm()->GetBookmarkManager().GetEditSession().SetIsVisible(static_cast<kml::MarkGroupId>(catId), isVisible);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeSetCategoryName(
    JNIEnv * env, jobject thiz, jlong catId, jstring name)
{
  frm()->GetBookmarkManager().GetEditSession().SetCategoryName(static_cast<kml::MarkGroupId>(catId),
                                                               jni::ToNativeString(env, name));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeSetCategoryDescription(JNIEnv * env,
                                                                                     jobject thiz,
                                                                                     jlong catId,
                                                                                     jstring desc)
{
  frm()->GetBookmarkManager().GetEditSession().SetCategoryDescription(
      static_cast<kml::MarkGroupId>(catId), jni::ToNativeString(env, desc));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeSetCategoryTags(
    JNIEnv * env, jobject, jlong catId, jobjectArray tagsIds)
{
  auto const size = env->GetArrayLength(tagsIds);
  std::vector<std::string> categoryTags;
  categoryTags.reserve(static_cast<size_t>(size));
  for (auto i = 0; i < size; i++)
  {
    jni::TScopedLocalRef const item(env, env->GetObjectArrayElement(tagsIds, i));
    categoryTags.push_back(jni::ToNativeString(env, static_cast<jstring>(item.get())));
  }

  frm()->GetBookmarkManager().GetEditSession().SetCategoryTags(static_cast<kml::MarkGroupId>(catId),
                                                               categoryTags);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeSetCategoryAccessRules(
    JNIEnv * env, jobject, jlong catId, jint accessRules)
{
  frm()->GetBookmarkManager().GetEditSession().SetCategoryAccessRules(
    static_cast<kml::MarkGroupId>(catId), static_cast<kml::AccessRules>(accessRules));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeSetCategoryCustomProperty(
    JNIEnv * env, jobject, jlong catId, jstring key, jstring value)
{
  frm()->GetBookmarkManager().GetEditSession().SetCategoryCustomProperty(
    static_cast<kml::MarkGroupId>(catId), ToNativeString(env, key), ToNativeString(env, value));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetCategoryName(
     JNIEnv * env, jobject thiz, jlong catId)
{
  return ToJavaString(env, frm()->GetBookmarkManager().GetCategoryName(
                      static_cast<kml::MarkGroupId>(catId)));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetCategoryAuthor(
        JNIEnv * env, jobject thiz, jlong catId)
{
  auto const & data = frm()->GetBookmarkManager().GetCategoryData(
    static_cast<kml::MarkGroupId>(catId));
  return ToJavaString(env, data.m_authorName);
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetBookmarksCount(
     JNIEnv * env, jobject thiz, jlong catId)
{
  return frm()->GetBookmarkManager().GetUserMarkIds(static_cast<kml::MarkGroupId>(catId)).size();
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetTracksCount(
     JNIEnv * env, jobject thiz, jlong catId)
{
  return frm()->GetBookmarkManager().GetTrackIds(static_cast<kml::MarkGroupId>(catId)).size();
}

// TODO(AlexZ): Get rid of UserMarks completely in UI code.
// TODO(yunikkk): Refactor java code to get all necessary info without Bookmark wrapper, and without hierarchy.
// If bookmark information is needed in the BookmarkManager, it does not relate in any way to Place Page info
// and should be passed separately via simple name string and lat lon to calculate a distance.
JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetBookmark(
     JNIEnv * env, jobject thiz, jlong bmkId)
{
  auto const * mark = frm()->GetBookmarkManager().GetBookmark(static_cast<kml::MarkId>(bmkId));
  place_page::Info info;
  frm()->FillBookmarkInfo(*mark, info);
  return usermark_helper::CreateMapObject(env, info);
}

JNIEXPORT jlong JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetBookmarkIdByPosition(
        JNIEnv * env, jobject thiz, jlong catId, jint positionInCategory)
{
  auto & ids = frm()->GetBookmarkManager().GetUserMarkIds(static_cast<kml::MarkGroupId>(catId));
  if (positionInCategory >= ids.size())
    return static_cast<jlong>(kml::kInvalidMarkId);
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
  auto const * nTrack = frm()->GetBookmarkManager().GetTrack(static_cast<kml::TrackId>(trackId));

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
  auto & ids = frm()->GetBookmarkManager().GetTrackIds(static_cast<kml::MarkGroupId>(catId));
  if (positionInCategory >= ids.size())
    return static_cast<jlong>(kml::kInvalidTrackId);
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
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeIsEditableBookmark(
        JNIEnv * env, jobject thiz, jlong bmkId)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().IsEditableBookmark(static_cast<kml::MarkId>(bmkId)));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeIsEditableTrack(
        JNIEnv * env, jobject thiz, jlong trackId)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().IsEditableTrack(static_cast<kml::TrackId>(trackId)));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeIsEditableCategory(
        JNIEnv * env, jobject thiz, jlong catId)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().IsEditableCategory(static_cast<kml::MarkGroupId>(catId)));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeAreAllCategoriesInvisible(
        JNIEnv * env, jobject thiz, jint type)
{
  auto const value = static_cast<BookmarkManager::CategoryFilterType>(type);
  return static_cast<jboolean>(frm()->GetBookmarkManager().AreAllCategoriesInvisible(value));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeAreAllCategoriesVisible(
        JNIEnv * env, jobject thiz, jint type)
{
  auto const value = static_cast<BookmarkManager::CategoryFilterType>(type);
  return static_cast<jboolean>(frm()->GetBookmarkManager().AreAllCategoriesVisible(value));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeSetAllCategoriesVisibility(
        JNIEnv * env, jobject thiz, jboolean visible, jint type)
{
  auto const filter = static_cast<BookmarkManager::CategoryFilterType>(type);
  frm()->GetBookmarkManager().SetAllCategoriesVisibility(filter, static_cast<bool>(visible));
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
  frm()->GetBookmarkManager().PrepareFileForSharing(static_cast<kml::MarkGroupId>(catId),
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
    static_cast<kml::MarkGroupId>(catId)));
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

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeSetNotificationsEnabled(
        JNIEnv * env, jobject thiz, jboolean enabled)
{
  frm()->GetBookmarkManager().SetNotificationsEnabled(static_cast<bool>(enabled));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeAreNotificationsEnabled(
        JNIEnv * env, jobject thiz)
{
  return static_cast<jboolean>(frm()->GetBookmarkManager().AreNotificationsEnabled());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeImportFromCatalog(
        JNIEnv * env, jobject, jstring serverId, jstring filePath)
{
  auto & bm = frm()->GetBookmarkManager();
  bm.ImportDownloadedFromCatalog(ToNativeString(env, serverId), ToNativeString(env, filePath));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeUploadToCatalog(
        JNIEnv * env, jobject, jint accessRules, jlong catId)
{
  auto & bm = frm()->GetBookmarkManager();
  bm.UploadToCatalog(static_cast<kml::MarkGroupId>(catId),
                     static_cast<kml::AccessRules>(accessRules));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetCatalogDeeplink(
        JNIEnv * env, jobject, jlong catId)
{
  auto & bm = frm()->GetBookmarkManager();
  return ToJavaString(env, bm.GetCategoryCatalogDeeplink(static_cast<kml::MarkGroupId>(catId)));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetCatalogDownloadUrl(
        JNIEnv * env, jobject, jstring serverId)
{
  auto & bm = frm()->GetBookmarkManager();
  return ToJavaString(env, bm.GetCatalog().GetDownloadUrl(ToNativeString(env, serverId)));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetCatalogFrontendUrl(
        JNIEnv * env, jobject)
{
  auto & bm = frm()->GetBookmarkManager();
  return ToJavaString(env, bm.GetCatalog().GetFrontendUrl());
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeIsCategoryFromCatalog(
        JNIEnv *, jobject, jlong catId)
{
  auto & bm = frm()->GetBookmarkManager();
  return static_cast<jboolean>(bm.IsCategoryFromCatalog(static_cast<kml::MarkGroupId>(catId)));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeRequestCatalogTags(
        JNIEnv * env, jobject)
{
  auto & bm = frm()->GetBookmarkManager();
  bm.GetCatalog().RequestTagGroups(languages::GetCurrentNorm(),
    [env](bool successful, BookmarkCatalog::TagGroups const & groups)
  {
    OnTagsReceived(env, successful, groups);
  });
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeRequestCatalogCustomProperties(
        JNIEnv * env, jobject)
{
  auto & bm = frm()->GetBookmarkManager();
  bm.GetCatalog().RequestCustomProperties(languages::GetCurrentNorm(),
    [env](bool successful, BookmarkCatalog::CustomProperties const & properties)
  {
    OnCustomPropertiesReceived(env, successful, properties);
  });
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetBookmarkCategories(JNIEnv *env, jobject thiz)
{
  auto const & bm = frm()->GetBookmarkManager();
  kml::GroupIdCollection const & categories = bm.GetBmGroupsIdList();
  auto const bookmarkConverter = [](JNIEnv * env, kml::MarkGroupId const & item)
  {
    auto const & manager = frm()->GetBookmarkManager();
    auto const & data = manager.GetCategoryData(item);
    auto const isFromCatalog = manager.IsCategoryFromCatalog(item);
    auto const tracksCount = manager.GetTrackIds(data.m_id).size();
    auto const bookmarksCount = manager.GetUserMarkIds(data.m_id).size();
    auto const isMyCategory = manager.IsMyCategory(item);
    auto const isVisible = manager.IsVisible(data.m_id);
    auto const preferBookmarkStr = GetPreferredBookmarkStr(data.m_name);
    auto const annotation = GetPreferredBookmarkStr(data.m_annotation);
    auto const description = GetPreferredBookmarkStr(data.m_description);
    auto const accessRules = data.m_accessRules;

    jni::TScopedLocalRef preferBookmarkStrRef(env, jni::ToJavaString(env, preferBookmarkStr));
    jni::TScopedLocalRef authorIdRef(env, jni::ToJavaString(env, data.m_authorId));
    jni::TScopedLocalRef authorNameRef(env, jni::ToJavaString(env, data.m_authorName));
    jni::TScopedLocalRef annotationRef(env, jni::ToJavaString(env, annotation));
    jni::TScopedLocalRef descriptionRef(env, jni::ToJavaString(env, description));

    return env->NewObject(g_bookmarkCategoryClass,
                          g_bookmarkCategoryConstructor,
                          static_cast<jlong>(data.m_id),
                          preferBookmarkStrRef.get(),
                          authorIdRef.get(),
                          authorNameRef.get(),
                          annotationRef.get(),
                          descriptionRef.get(),
                          static_cast<jint>(tracksCount),
                          static_cast<jint>(bookmarksCount),
                          static_cast<jboolean>(isFromCatalog),
                          static_cast<jboolean>(isMyCategory),
                          static_cast<jboolean>(isVisible),
                          static_cast<jint>(accessRules));
  };
  return ToJavaArray(env, g_bookmarkCategoryClass, categories, bookmarkConverter);
}
}  // extern "C"
