#include "../../Framework.hpp"

#include "../../../core/jni_helper.hpp"

namespace {
  ::Framework * frm() { return g_framework->NativeFramework(); }
}

extern "C"
{

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetName(
       JNIEnv * env, jobject thiz, jstring cat, jlong bmk)
  {
    return jni::ToJavaString(env, frm()->GetBmCategory(jni::ToNativeString(env, cat))->GetBookmark(bmk)->GetName());
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetNamePos(
         JNIEnv * env, jobject thiz, jint x, jint y)
  {
    BookmarkAndCategory bc = frm()->GetBookmark(m2::PointD(x, y));
    int cat = bc.first;
    int bmk = bc.second;
    return jni::ToJavaString(env, frm()->GetBmCategory(cat)->GetBookmark(bmk)->GetName());
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetIcon(
       JNIEnv * env, jobject thiz, jstring cat, jlong bmk)
  {
    return jni::ToJavaString(env, frm()->GetBmCategory(jni::ToNativeString(env, cat))->GetBookmark(bmk)->GetType());
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetIconPos(
         JNIEnv * env, jobject thiz, jint x, jint y)
  {
    BookmarkAndCategory bc = frm()->GetBookmark(m2::PointD(x, y));
    int cat = bc.first;
    int bmk = bc.second;
    return jni::ToJavaString(env, frm()->GetBmCategory(cat)->GetBookmark(bmk)->GetType());
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nChangeBookamark(
         JNIEnv * env, jobject thiz, jdouble lat, jdouble lon, jstring cat, jstring name, jstring type)
  {
    frm()->AddBookmark(jni::ToNativeString(env, cat), Bookmark( m2::PointD(lat, lon), jni::ToNativeString(env, name), jni::ToNativeString(env, type)));
  }

  JNIEXPORT jdoubleArray JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetLatLon(
       JNIEnv * env, jobject thiz, jstring cat, jlong bmk)
  {
    m2::PointD org = frm()->GetBmCategory(jni::ToNativeString(env, cat))->GetBookmark(bmk)->GetOrg();
    jdoubleArray result;
    result = env->NewDoubleArray(2);
    if (result == NULL) {
        return NULL; /* out of memory error thrown */
    }
    double fill[2];
    fill[0] = org.x;
    fill[1] = org.y;
    env->SetDoubleArrayRegion(result, 0, 2, fill);
    return result;
  }

}
