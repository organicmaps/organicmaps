#include "android/jni/com/mapswithme/maps/Framework.hpp"

#include "android/jni/com/mapswithme/core/jni_helper.hpp"


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
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeGetName(
     JNIEnv * env, jobject thiz, jint cat, jlong bmk)
{
  return jni::ToJavaString(env, getBookmark(cat, bmk)->GetName());
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeGetBookmarkDescription(
     JNIEnv * env, jobject thiz, jint cat, jlong bmk)
{
  return jni::ToJavaString(env, getBookmark(cat, bmk)->GetDescription());
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeGetIcon(
     JNIEnv * env, jobject thiz, jint cat, jlong bmk)
{
  return jni::ToJavaString(env, getBookmark(cat, bmk)->GetType());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeSetBookmarkParams(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk,
       jstring name, jstring type, jstring descr)
{
  Bookmark const * p = getBookmark(cat, bmk);

  // initialize new bookmark
  BookmarkData bm(jni::ToNativeString(env, name), jni::ToNativeString(env, type));
  bm.SetDescription(descr ? jni::ToNativeString(env, descr)
                          : p->GetDescription());

  g_framework->ReplaceBookmark(BookmarkAndCategory(bmk, cat), bm);
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeChangeCategory(
       JNIEnv * env, jobject thiz, jint oldCat, jint newCat, jlong bmk)
{
  return g_framework->ChangeBookmarkCategory(BookmarkAndCategory(bmk, oldCat), newCat);
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeGetXY(
     JNIEnv * env, jobject thiz, jint cat, jlong bmk)
{
  return jni::GetNewParcelablePointD(env, getBookmark(cat, bmk)->GetPivot());
}

JNIEXPORT jdouble JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeGetScale(
     JNIEnv * env, jobject thiz, jint cat, jlong bmk)
{
  return getBookmark(cat, bmk)->GetScale();
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeEncode2Ge0Url(
     JNIEnv * env, jobject thiz, jint cat, jlong bmk, jboolean addName)
{
  return jni::ToJavaString(env, frm()->CodeGe0url(getBookmark(cat, bmk), addName));
}
} // extern "C"
