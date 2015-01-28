#include "../../Framework.hpp"

#include "../../../core/jni_helper.hpp"

#include "coding/zip_creator.hpp"


namespace
{
  ::Framework * frm() { return g_framework->NativeFramework(); }
}

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_showBookmarkOnMap(
      JNIEnv * env, jobject thiz, jint c, jint b)
  {
    BookmarkAndCategory bnc = BookmarkAndCategory(c,b);
    frm()->ShowBookmark(bnc);
    frm()->SaveState();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_loadBookmarks(JNIEnv * env, jobject thiz)
  {
    frm()->LoadBookmarks();
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_getCategoriesCount(JNIEnv * env, jobject thiz)
  {
    return frm()->GetBmCategoriesCount();
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_createCategory(
       JNIEnv * env, jobject thiz, jstring name)
  {
    return frm()->AddCategory(jni::ToNativeString(env, name));
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_deleteCategory(
       JNIEnv * env, jobject thiz, jint index)
  {
    return frm()->DeleteBmCategory(index) ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_deleteBookmark(
      JNIEnv * env, jobject thiz, jint cat, jint bmk)
  {
    BookmarkCategory * pCat = frm()->GetBmCategory(cat);
    if (pCat)
    {
      BookmarkCategory::Guard guard(*pCat);
      guard.m_controller.DeleteUserMark(bmk);
      pCat->SaveToKMLFile();
    }
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nativeDeleteTrack(
      JNIEnv * env, jobject thiz, jint cat, jint trk)
  {
    BookmarkCategory * pCat = frm()->GetBmCategory(cat);
    if (pCat)
    {
      pCat->DeleteTrack(trk);
      pCat->SaveToKMLFile();
    }
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_saveToKmzFile(
      JNIEnv * env, jobject thiz, jint catID, jstring tmpPath)
  {
    BookmarkCategory * pCat = frm()->GetBmCategory(catID);
    if (pCat)
    {
      string const name = pCat->GetName();
      if (CreateZipFromPathDeflatedAndDefaultCompression(pCat->GetFileName(), jni::ToNativeString(env, tmpPath) + name + ".kmz"))
        return jni::ToJavaString(env, name);
    }

    return 0;
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_addBookmarkToLastEditedCategory(
      JNIEnv * env, jobject thiz, jstring name, double lat, double lon)
  {
    const m2::PointD glbPoint(MercatorBounds::FromLatLon(lat, lon));

    ::Framework * f = frm();
    BookmarkData bmk(jni::ToNativeString(env, name), f->LastEditedBMType());
    return g_framework->AddBookmark(f->LastEditedBMCategory(), glbPoint, bmk).second;
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_getLastEditedCategory(
        JNIEnv * env, jobject thiz)
  {
    return frm()->LastEditedBMCategory();
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_generateUniqueBookmarkName(JNIEnv * env, jclass thiz, jstring jBaseName)
  {
    string baseName = jni::ToNativeString(env, jBaseName);
    string bookmarkFileName = BookmarkCategory::GenerateUniqueFileName(GetPlatform().SettingsDir(), baseName);
    return jni::ToJavaString(env, bookmarkFileName);
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_loadKmzFile(JNIEnv * env, jobject thiz, jstring path)
  {
    return frm()->AddBookmarksFile(jni::ToNativeString(env, path)) ? JNI_TRUE : JNI_FALSE;
  }
}
