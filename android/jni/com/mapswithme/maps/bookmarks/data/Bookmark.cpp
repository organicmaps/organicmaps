#include "../../Framework.hpp"

#include "../../../core/jni_helper.hpp"


namespace
{
  ::Framework * frm() { return g_framework->NativeFramework(); }

  Bookmark const * getBookmark(jint c, jlong b)
  {
    BookmarkCategory const * pCat = frm()->GetBmCategory(c);
    ASSERT(pCat, ("Category not found", c));
    Bookmark const * pBmk = static_cast<Bookmark const *>(pCat->GetUserMark(b));
    return pBmk;
  }
}

extern "C"
{
  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_getName(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    return jni::ToJavaString(env, getBookmark(cat, bmk)->GetName());
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_getBookmarkDescription(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    return jni::ToJavaString(env, getBookmark(cat, bmk)->GetDescription());
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_getIcon(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    return jni::ToJavaString(env, getBookmark(cat, bmk)->GetType());
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_setBookmarkParams(
         JNIEnv * env, jobject thiz, jint cat, jlong bmk,
         jstring name, jstring type, jstring descr)
  {
    Bookmark const * p = getBookmark(cat, bmk);

    // initialize new bookmark
    BookmarkData bm(jni::ToNativeString(env, name), jni::ToNativeString(env, type));
    if (descr != 0)
      bm.SetDescription(jni::ToNativeString(env, descr));
    else
      bm.SetDescription(p->GetDescription());

    g_framework->ReplaceBookmark(BookmarkAndCategory(cat, bmk), bm);
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_changeCategory(
         JNIEnv * env, jobject thiz, jint oldCat, jint newCat, jlong bmk)
  {
    return g_framework->ChangeBookmarkCategory(BookmarkAndCategory(oldCat, bmk), newCat);
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_getXY(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    return jni::GetNewParcelablePointD(env, getBookmark(cat, bmk)->GetOrg());
  }

  JNIEXPORT jdouble JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_getScale(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    return getBookmark(cat, bmk)->GetScale();
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_encode2Ge0Url(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk, jboolean addName)
  {
    return jni::ToJavaString(env, frm()->CodeGe0url(getBookmark(cat, bmk), addName));
  }
}
