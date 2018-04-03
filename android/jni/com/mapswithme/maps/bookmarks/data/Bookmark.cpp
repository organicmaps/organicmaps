#include "com/mapswithme/maps/Framework.hpp"

#include "com/mapswithme/core/jni_helper.hpp"


namespace
{
::Framework * frm() { return g_framework->NativeFramework(); }

Bookmark const * getBookmark(jlong bokmarkId)
{
  Bookmark const * pBmk = frm()->GetBookmarkManager().GetBookmark(static_cast<kml::MarkId>(bokmarkId));
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

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeGetColor(
     JNIEnv * env, jobject thiz, jlong bmk)
{
  auto const * mark = getBookmark(bmk);
  return static_cast<jint>(mark != nullptr ? mark->GetColor()
                                           : frm()->LastEditedBMColor());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeSetBookmarkParams(
       JNIEnv * env, jobject thiz, jlong bmk,
       jstring name, jint color, jstring descr)
{
  auto const * mark = getBookmark(bmk);

  // initialize new bookmark
  kml::BookmarkData bmData(mark->GetData());
  kml::SetDefaultStr(bmData.m_name, jni::ToNativeString(env, name));
  if (descr)
    kml::SetDefaultStr(bmData.m_description, jni::ToNativeString(env, descr));
  bmData.m_color.m_predefinedColor = static_cast<kml::PredefinedColor>(color);

  g_framework->ReplaceBookmark(static_cast<kml::MarkId>(bmk), bmData);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_Bookmark_nativeChangeCategory(
       JNIEnv * env, jobject thiz, jlong oldCat, jlong newCat, jlong bmk)
{
  g_framework->MoveBookmark(static_cast<kml::MarkId>(bmk), static_cast<kml::MarkGroupId>(oldCat),
                            static_cast<kml::MarkGroupId>(newCat));
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
