#include "../../Framework.hpp"

#include "../../../core/jni_helper.hpp"
#include "../../../../../../../base/logging.hpp"

namespace {
  ::Framework * frm() { return g_framework->NativeFramework(); }
}

extern "C"
{
  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGtoP(JNIEnv * env, jobject thiz, jdouble lat, jdouble lon)
  {
    return jni::GetNewPoint(env, frm()->GtoP(m2::PointD(lat, lon)));
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetName(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    return jni::ToJavaString(env, frm()->GetBmCategory(cat)->GetBookmark(bmk)->GetName());
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetBookmarkDescription(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    string descr = frm()->GetBmCategory(cat)->GetBookmark(bmk)->GetDescription();
    LOG(LDEBUG,("Description get ",descr));
    return jni::ToJavaString(env, descr);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nSetBookmarkDescription(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk, jstring newDescr)
  {
    Bookmark b(* (frm()->GetBmCategory(cat)->GetBookmark(bmk)) );
    b.SetDescription(jni::ToNativeString(env, newDescr));
    LOG(LDEBUG,("Description ",b.GetDescription()));
    frm()->AddBookmark(frm()->GetBmCategory(cat)->GetName(), b);
    frm()->GetBmCategory(frm()->GetBmCategory(cat)->GetName())->SaveToKMLFile();
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
    BookmarkAndCategory bac = frm()->GetBookmark(frm()->GtoP(m2::PointD(lan, lon)));
    Bookmark b = Bookmark( m2::PointD(lan, lon), jni::ToNativeString(env, name), jni::ToNativeString(env, type));
    if (bac.first > -1 && bac.second > -1)
    {
      const Bookmark bp = * frm()->GetBmCategory(bac.first)->GetBookmark(bac.second);
      b.SetDescription(bp.GetDescription());
    }
    frm()->AddBookmark(jni::ToNativeString(env, cat), b)->SaveToKMLFile();
    frm()->GetBmCategory(bac.first)->SaveToKMLFile();
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nPtoG(
       JNIEnv * env, jobject thiz, jint x, jint y)
  {
    return jni::GetNewParcelablePointD(env, frm()->PtoG(m2::PointD(x, y)));
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetDistanceAndAzimuth(
      JNIEnv * env, jobject thiz, jdouble lat, jdouble lon, jdouble cLat, jdouble cLon, jdouble north)
  {
    jclass klass = env->FindClass("com/mapswithme/maps/bookmarks/data/DistanceAndAthimuth");
    ASSERT ( klass, () );
    jmethodID methodID = env->GetMethodID(
        klass, "<init>",
        "(Ljava/lang/String;D)V");
    ASSERT ( methodID, () );

    string distance;
    double azimut = -1.0;
    g_framework->NativeFramework()->GetDistanceAndAzimut(
        m2::PointD(lat, lon), cLat, cLon, north, distance, azimut);

    return env->NewObject(klass, methodID,
                          jni::ToJavaString(env, distance.c_str()),
                          static_cast<jdouble>(azimut));
  }
  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetLatLon(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    return jni::GetNewParcelablePointD(env, frm()->GetBmCategory(cat)->GetBookmark(bmk)->GetOrg());
  }

}
