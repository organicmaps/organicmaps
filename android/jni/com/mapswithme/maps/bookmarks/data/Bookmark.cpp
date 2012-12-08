#include "../../Framework.hpp"

#include "../../../core/jni_helper.hpp"
#include "../../../../../../../base/logging.hpp"

namespace {
  ::Framework * frm() { return g_framework->NativeFramework(); }
}

extern "C"
{

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetName(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    return jni::ToJavaString(env, frm()->GetBmCategory(cat)->GetBookmark(bmk)->GetName());
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
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    return jni::ToJavaString(env, frm()->GetBmCategory(cat)->GetBookmark(bmk)->GetType());
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
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nChangeBookmark(
         JNIEnv * env, jobject thiz, jdouble lan, jdouble lon, jstring cat, jstring name, jstring type)
  {
    LOG(LDEBUG, ("lan lon change", lan, lon));
    frm()->AddBookmark(jni::ToNativeString(env, cat), Bookmark( m2::PointD(lan, lon), jni::ToNativeString(env, name), jni::ToNativeString(env, type)));
  }

  JNIEXPORT jdoubleArray JNICALL
    Java_com_mapswithme_maps_bookmarks_data_Bookmark_nPtoG(
         JNIEnv * env, jobject thiz, jint x, jint y)
    {
      m2::PointD org = frm()->PtoG(m2::PointD(x, y));
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

  JNIEXPORT jdoubleArray JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetLatLon(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    m2::PointD org = frm()->GetBmCategory(cat)->GetBookmark(bmk)->GetOrg();
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
