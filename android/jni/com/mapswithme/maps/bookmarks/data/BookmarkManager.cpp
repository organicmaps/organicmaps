#include "../../Framework.hpp"

#include "../../../core/jni_helper.hpp"

namespace
{
  ::Framework * frm() { return g_framework->NativeFramework(); }
}

extern "C"
{
  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nGetPOI(JNIEnv * env, jobject thiz, jdouble px, jdouble py)
  {
    Framework::AddressInfo adInfo;
    m2::PointD pxPivot;
    if (frm()->GetVisiblePOI(m2::PointD(px, py), pxPivot, adInfo))
    {
      return jni::GetNewAddressInfo(env, adInfo, pxPivot);
    }
    else
    {
      return env->NewGlobalRef(NULL);
    }
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nGetAddressInfo(JNIEnv * env, jobject thiz, jdouble px, jdouble py)
  {
    m2::PointD point(px, py);
    Framework::AddressInfo adInfo;
    frm()->GetAddressInfo(point, adInfo);
    return jni::GetNewAddressInfo(env, adInfo, point);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nShowBookmark(JNIEnv * env, jobject thiz, jint c, jint b)
  {
    frm()->ShowBookmark(*(frm()->GetBmCategory(c)->GetBookmark(b)));
    frm()->SaveState();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_putBookmark(
      JNIEnv * env, jobject thiz, jint px, jint py, jstring bookmarkName, jstring categoryName)
  {
    Bookmark bm(frm()->PtoG(m2::PointD(px, py)), jni::ToNativeString(env, bookmarkName), "placemark-red");
    frm()->AddBookmark(jni::ToNativeString(env, categoryName), bm)->SaveToKMLFile();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nLoadBookmarks(
      JNIEnv * env, jobject thiz)
  {
    frm()->LoadBookmarks();
  }


  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_getCategoriesCount(
       JNIEnv * env, jobject thiz)
  {
    return frm()->GetBmCategoriesCount();
  }

  //TODO rename
  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nGetCategoryByName(
      JNIEnv * env, jobject thiz, jstring name)
 {
   return frm()->IsCategoryExist(jni::ToNativeString(env, name));
 }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nDeleteCategory(
       JNIEnv * env, jobject thiz, jint index)
  {
    return frm()->DeleteBmCategory(index);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nDeleteBookmark(JNIEnv * env, jobject thiz, jint cat, jint bmk)
  {
    BookmarkCategory * pCat = frm()->GetBmCategory(cat);
    if (pCat)
    {
      pCat->DeleteBookmark(bmk);
      pCat->SaveToKMLFile();
    }
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nGetBookmark(JNIEnv * env, jobject thiz, jdouble px, jdouble py)
  {
    BookmarkAndCategory bac = frm()->GetBookmark(m2::PointD(px, py));

    return jni::GetNewPoint(env, m2::PointI(bac.first, bac.second));
   }
}
