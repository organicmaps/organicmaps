#include "../../Framework.hpp"

#include "../../../core/jni_helper.hpp"

#include "../../../../../../../coding/zip_creator.hpp"


namespace
{
  ::Framework * frm() { return g_framework->NativeFramework(); }
}

extern "C"
{
  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_getPOI(
      JNIEnv * env, jobject thiz, jdouble px, jdouble py)
  {
    Framework::AddressInfo addrInfo;
    m2::PointD pxPivot;
    if (frm()->GetVisiblePOI(m2::PointD(px, py), pxPivot, addrInfo))
      return jni::GetNewAddressInfo(env, addrInfo, pxPivot);
    else
      return 0;
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_getAddressInfo(
      JNIEnv * env, jobject thiz, jdouble px, jdouble py)
  {
    m2::PointD point(px, py);
    Framework::AddressInfo addrInfo;
    frm()->GetAddressInfo(point, addrInfo);
    return jni::GetNewAddressInfo(env, addrInfo, point);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_showBookmarkOnMap(
      JNIEnv * env, jobject thiz, jint c, jint b)
  {
    frm()->ShowBookmark(*(frm()->GetBmCategory(c)->GetBookmark(b)));
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

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_isCategoryExist(
      JNIEnv * env, jobject thiz, jstring name)
  {
    return frm()->IsCategoryExist(jni::ToNativeString(env, name));
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_deleteCategory(
       JNIEnv * env, jobject thiz, jint index)
  {
    return frm()->DeleteBmCategory(index);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_deleteBookmark(
      JNIEnv * env, jobject thiz, jint cat, jint bmk)
  {
    BookmarkCategory * pCat = frm()->GetBmCategory(cat);
    if (pCat)
    {
      pCat->DeleteBookmark(bmk);
      pCat->SaveToKMLFile();
    }
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_getBookmark(
      JNIEnv * env, jobject thiz, jdouble px, jdouble py)
  {
    BookmarkAndCategory const bac = frm()->GetBookmark(m2::PointD(px, py));
    return jni::GetNewPoint(env, m2::PointI(bac.first, bac.second));
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_saveToKMZFile(
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
}
