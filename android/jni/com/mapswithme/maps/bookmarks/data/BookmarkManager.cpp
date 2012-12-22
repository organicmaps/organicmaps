#include "../../Framework.hpp"

#include "../../../core/jni_helper.hpp"

namespace {
  ::Framework * frm() { return g_framework->NativeFramework(); }
}

extern "C"
{
  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nShowBookmark(JNIEnv * env, jobject thiz, jint c, jint b)
  {
    frm()->ShowBookmark(*g_framework->NativeFramework()->GetBmCategory(c)->GetBookmark(b));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_putBookmark(
      JNIEnv * env, jobject thiz, jint x, jint y, jstring bookmarkName, jstring categoryName)
  {
    Bookmark bm(frm()->PtoG(m2::PointD(x, y)), jni::ToNativeString(env, bookmarkName), "placemark-red");
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
    frm()->GetBmCategory(cat)->DeleteBookmark(bmk);
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkManager_nGetBookmark(JNIEnv * env, jobject thiz, jint x, jint y)
  {
    BookmarkAndCategory bac = frm()->GetBookmark(m2::PointD(x, y));

    return jni::GetNewPoint(env, m2::PointI(bac.first, bac.second));
   }
}
