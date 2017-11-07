#include "BookmarkManager.hpp"

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
jmethodID g_onBookmarksLoadingFileMethod;

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
  g_onBookmarksLoadingFileMethod =
    jni::GetMethodID(env, bookmarkManagerInstance, "onBookmarksLoadingFile",
                     "(ZLjava/lang/String;Z)V");
}

void OnAsyncLoadingStarted(JNIEnv * env)
{
  ASSERT(g_bookmarkManagerClass != nullptr, ());
  LOG(LINFO, ("!!!!!!"));
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onBookmarksLoadingStartedMethod);
  jni::HandleJavaException(env);
}

void OnAsyncLoadingFinished(JNIEnv * env)
{
  ASSERT(g_bookmarkManagerClass != nullptr, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  env->CallVoidMethod(bookmarkManagerInstance, g_onBookmarksLoadingFinishedMethod);
  jni::HandleJavaException(env);
}

void OnAsyncLoadingFileSuccess(JNIEnv * env, std::string const & fileName, bool isTemporaryFile)
{
  ASSERT(g_bookmarkManagerClass != nullptr, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  jni::TScopedLocalRef jFileName(env, jni::ToJavaString(env, fileName));
  env->CallVoidMethod(bookmarkManagerInstance, g_onBookmarksLoadingFileMethod,
                      true /* success */, jFileName.get(), isTemporaryFile);
  jni::HandleJavaException(env);
}

void OnAsyncLoadingFileError(JNIEnv * env, std::string const & fileName, bool isTemporaryFile)
{
  ASSERT(g_bookmarkManagerClass != nullptr, ());
  jobject bookmarkManagerInstance = env->GetStaticObjectField(g_bookmarkManagerClass,
                                                              g_bookmarkManagerInstanceField);
  jni::TScopedLocalRef jFileName(env, jni::ToJavaString(env, fileName));
  env->CallVoidMethod(bookmarkManagerInstance, g_onBookmarksLoadingFileMethod,
                      false /* success */, jFileName.get(), isTemporaryFile);
  jni::HandleJavaException(env);
}
}  // namespace

namespace bookmarks_helper
{
void RemoveBookmark(int cat, int bmk)
{
  BookmarkCategory * pCat = frm()->GetBmCategory(cat);
  if (pCat)
  {
    pCat->DeleteUserMark(bmk);
    pCat->NotifyChanges();
    pCat->SaveToKMLFile();
  }
}
}  // namespace bookmarks_helper

extern "C"
{
using namespace jni;

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeShowBookmarkOnMap(
    JNIEnv * env, jobject thiz, jint c, jint b)
{
  BookmarkAndCategory bnc = BookmarkAndCategory(b, c);
  frm()->ShowBookmark(bnc);
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

  frm()->LoadBookmarks();
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetCategoriesCount(JNIEnv * env, jobject thiz)
{
  return frm()->GetBmCategoriesCount();
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeCreateCategory(
     JNIEnv * env, jobject thiz, jstring name)
{
  return frm()->AddCategory(ToNativeString(env, name));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeDeleteCategory(
     JNIEnv * env, jobject thiz, jint index)
{
  return frm()->DeleteBmCategory(index);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeDeleteBookmark(JNIEnv *, jobject, jint cat, jint bmk)
{
  bookmarks_helper::RemoveBookmark(cat, bmk);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeDeleteTrack(
    JNIEnv * env, jobject thiz, jint cat, jint trk)
{
  BookmarkCategory * pCat = frm()->GetBmCategory(cat);
  if (pCat)
  {
    pCat->DeleteTrack(trk);
    pCat->NotifyChanges();
    pCat->SaveToKMLFile();
  }
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeSaveToKmzFile(
    JNIEnv * env, jobject thiz, jint catID, jstring tmpPath)
{
  BookmarkCategory * pCat = frm()->GetBmCategory(catID);
  if (pCat)
  {
    std::string const name = pCat->GetName();
    if (CreateZipFromPathDeflatedAndDefaultCompression(pCat->GetFileName(), ToNativeString(env, tmpPath) + name + ".kmz"))
      return ToJavaString(env, name);
  }

  return nullptr;
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeAddBookmarkToLastEditedCategory(
    JNIEnv * env, jobject thiz, jstring name, double lat, double lon)
{
  m2::PointD const glbPoint(MercatorBounds::FromLatLon(lat, lon));
  ::Framework * f = frm();
  BookmarkData bmkData(ToNativeString(env, name), f->LastEditedBMType());
  size_t const lastEditedCategory = f->LastEditedBMCategory();
  size_t const createdBookmarkIndex = f->AddBookmark(lastEditedCategory, glbPoint, bmkData);

  place_page::Info & info = g_framework->GetPlacePageInfo();
  BookmarkCategory * category = f->GetBmCategory(lastEditedCategory);
  f->FillBookmarkInfo(*static_cast<Bookmark const *>(category->GetUserMark(createdBookmarkIndex)),
                      {static_cast<size_t>(createdBookmarkIndex),
                       static_cast<size_t>(lastEditedCategory)}, info);

  return usermark_helper::CreateMapObject(env, info);
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGetLastEditedCategory(
      JNIEnv * env, jobject thiz)
{
  return frm()->LastEditedBMCategory();
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeGenerateUniqueFileName(JNIEnv * env, jclass thiz, jstring jBaseName)
{
  std::string baseName = ToNativeString(env, jBaseName);
  std::string bookmarkFileName = BookmarkCategory::GenerateUniqueFileName(GetPlatform().SettingsDir(), baseName);
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

}  // extern "C"
