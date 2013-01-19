#include "../../Framework.hpp"

#include "../../../core/jni_helper.hpp"
#include "../../../../../../../base/logging.hpp"

namespace
{
  ::Framework * frm() { return g_framework->NativeFramework(); }

  Bookmark const * getBookmark(jint c, jlong b)
  {
    BookmarkCategory const * pCat = frm()->GetBmCategory(c);
    ASSERT(pCat, ("Category not found", c));
    Bookmark const * pBmk = pCat->GetBookmark(b);
    ASSERT(pBmk, ("Bookmark not found", c, b));
    return pBmk;
  }
}

extern "C"
{
  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGtoP(JNIEnv * env, jobject thiz, jdouble x, jdouble y)
  {
    return jni::GetNewPoint(env, frm()->GtoP(m2::PointD(x, y)));
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetName(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    return jni::ToJavaString(env, getBookmark(cat, bmk)->GetName());
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetBookmarkDescription(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    return jni::ToJavaString(env, getBookmark(cat, bmk)->GetDescription());
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nSetBookmarkDescription(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk, jstring newDescr)
  {
    Bookmark b(*getBookmark(cat, bmk));
    b.SetDescription(jni::ToNativeString(env, newDescr));
    string name = frm()->GetBmCategory(cat)->GetName();
    frm()->AddBookmark(name, b);
    frm()->GetBmCategory(name)->SaveToKMLFile();
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetNamePos(
         JNIEnv * env, jobject thiz, jint x, jint y)
  {
    BookmarkAndCategory bc = frm()->GetBookmark(m2::PointD(x, y));
    return jni::ToJavaString(env, getBookmark(bc.first, bc.second)->GetName());
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetIcon(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    return jni::ToJavaString(env, getBookmark(cat, bmk)->GetType());
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetIconPos(
         JNIEnv * env, jobject thiz, jint x, jint y)
  {
    BookmarkAndCategory bc = frm()->GetBookmark(m2::PointD(x, y));
    return jni::ToJavaString(env, getBookmark(bc.first, bc.second)->GetType());
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nChangeBookmark(
         JNIEnv * env, jobject thiz, jdouble x, jdouble y, jstring cat, jstring name, jstring type)
  {
    BookmarkAndCategory bac = frm()->GetBookmark(frm()->GtoP(m2::PointD(x, y)));
    Bookmark b(m2::PointD(x, y), jni::ToNativeString(env, name), jni::ToNativeString(env, type));
    if (bac.first > -1 && bac.second > -1)
    {
      b.SetDescription(getBookmark(bac.first, bac.second)->GetDescription());
    }
    frm()->AddBookmark(jni::ToNativeString(env, cat), b)->SaveToKMLFile();
    if (bac.first >= 0)
      frm()->GetBmCategory(bac.first)->SaveToKMLFile();
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nPtoG(
       JNIEnv * env, jobject thiz, jint px, jint py)
  {
    return jni::GetNewParcelablePointD(env, frm()->PtoG(m2::PointD(px, py)));
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetDistanceAndAzimuth(
      JNIEnv * env, jobject thiz, jdouble x, jdouble y, jdouble cLat, jdouble cLon, jdouble north)
  {
    string distance;
    double azimut = -1.0;
    g_framework->NativeFramework()->GetDistanceAndAzimut(
        m2::PointD(x, y), cLat, cLon, north, distance, azimut);

    jclass klass = env->FindClass("com/mapswithme/maps/bookmarks/data/DistanceAndAthimuth");
    ASSERT ( klass, () );
    jmethodID methodID = env->GetMethodID(
        klass, "<init>",
        "(Ljava/lang/String;D)V");
    ASSERT ( methodID, () );

    return env->NewObject(klass, methodID,
                          jni::ToJavaString(env, distance.c_str()),
                          static_cast<jdouble>(azimut));
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_nGetXY(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    return jni::GetNewParcelablePointD(env, getBookmark(cat, bmk)->GetOrg());
  }

}
