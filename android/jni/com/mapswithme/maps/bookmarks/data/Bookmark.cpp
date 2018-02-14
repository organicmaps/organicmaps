#include "com/mapswithme/maps/Framework.hpp"

#include "com/mapswithme/core/jni_helper.hpp"


namespace
{
::Framework * frm() { return g_framework->NativeFramework(); }

Bookmark const * getBookmark(jlong bokmarkId)
{
  Bookmark const * pBmk = frm()->GetBookmarkManager().GetBookmark(static_cast<df::MarkID>(bokmarkId));
  ASSERT(pBmk, ("Bookmark not found, id", bokmarkId));
  return pBmk;
}
}

extern "C"
{
JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeGetName(
     JNIEnv * env, jobject thiz, jlong bmk)
{
  return jni::ToJavaString(env, getBookmark(bmk)->GetName());
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeGetBookmarkDescription(
     JNIEnv * env, jobject thiz, jlong bmk)
{
  return jni::ToJavaString(env, getBookmark(bmk)->GetDescription());
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeGetIcon(
     JNIEnv * env, jobject thiz, jlong bmk)
{
  return jni::ToJavaString(env, getBookmark(bmk)->GetType());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeSetBookmarkParams(
       JNIEnv * env, jobject thiz, jlong bmk,
       jstring name, jstring type, jstring descr)
{
  Bookmark const * p = getBookmark(bmk);

  // initialize new bookmark
  BookmarkData bm(jni::ToNativeString(env, name), jni::ToNativeString(env, type));
  bm.SetDescription(descr ? jni::ToNativeString(env, descr)
                          : p->GetDescription());

  g_framework->ReplaceBookmark(static_cast<df::MarkID>(bmk), bm);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeChangeCategory(
       JNIEnv * env, jobject thiz, jlong oldCat, jlong newCat, jlong bmk)
{
  g_framework->MoveBookmark(static_cast<df::MarkID>(bmk), static_cast<df::MarkGroupID>(oldCat),
                            static_cast<df::MarkGroupID>(newCat));
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeGetXY(
     JNIEnv * env, jobject thiz, jlong bmk)
{
  return jni::GetNewParcelablePointD(env, getBookmark(bmk)->GetPivot());
}

JNIEXPORT jdouble JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeGetScale(
     JNIEnv * env, jobject thiz, jlong bmk)
{
  return getBookmark(bmk)->GetScale();
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeEncode2Ge0Url(
     JNIEnv * env, jobject thiz, jlong bmk, jboolean addName)
{
  return jni::ToJavaString(env, frm()->CodeGe0url(getBookmark(bmk), addName));
}
} // extern "C"
