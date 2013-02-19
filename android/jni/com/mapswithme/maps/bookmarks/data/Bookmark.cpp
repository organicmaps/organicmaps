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
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_g2p(JNIEnv * env, jobject thiz, jdouble x, jdouble y)
  {
    return jni::GetNewParcelablePointD(env, frm()->GtoP(m2::PointD(x, y)));
  }

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

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_setBookmarkDescription(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk, jstring newDescr)
  {
    // do edit bookmark's description without AddBookmark routine
    Bookmark * pBM = const_cast<Bookmark *>(getBookmark(cat, bmk));
    pBM->SetDescription(jni::ToNativeString(env, newDescr));
    frm()->GetBmCategory(cat)->SaveToKMLFile();
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_getNamePos(
         JNIEnv * env, jobject thiz, jint px, jint py)
  {
    BookmarkAndCategory bc = frm()->GetBookmark(m2::PointD(px, py));
    return jni::ToJavaString(env, getBookmark(bc.first, bc.second)->GetName());
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_getIcon(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    return jni::ToJavaString(env, getBookmark(cat, bmk)->GetType());
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_getIconPos(
         JNIEnv * env, jobject thiz, jint px, jint py)
  {
    BookmarkAndCategory bc = frm()->GetBookmark(m2::PointD(px, py));
    return jni::ToJavaString(env, getBookmark(bc.first, bc.second)->GetType());
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_changeBookmark(
         JNIEnv * env, jobject thiz, jdouble x, jdouble y, jstring cat, jstring name, jstring type)
  {
    // get existing bookmark under point
    BookmarkAndCategory bac = frm()->GetBookmark(frm()->GtoP(m2::PointD(x, y)));

    // initialize new bookmark
    Bookmark bm(m2::PointD(x, y), jni::ToNativeString(env, name), jni::ToNativeString(env, type));
    if (IsValid(bac))
      bm.SetDescription(getBookmark(bac.first, bac.second)->GetDescription());

    // add new bookmark
    string const category = jni::ToNativeString(env, cat);
    g_framework->AddBookmark(category, bm);

    // save old bookmark's category
    if (bac.first > -1)
    {
      BookmarkCategory * pCat = frm()->GetBmCategory(bac.first);
      if (pCat->GetName() != category)
        pCat->SaveToKMLFile();
    }
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_p2g(
       JNIEnv * env, jobject thiz, jdouble px, jdouble py)
  {
    return jni::GetNewParcelablePointD(env, frm()->PtoG(m2::PointD(px, py)));
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_getDistanceAndAzimut(
      JNIEnv * env, jobject thiz, jdouble x, jdouble y, jdouble cLat, jdouble cLon, jdouble north)
  {
    string distance;
    double azimut = -1.0;
    g_framework->NativeFramework()->GetDistanceAndAzimut(
        m2::PointD(x, y), cLat, cLon, north, distance, azimut);

    jclass klass = env->FindClass("com/mapswithme/maps/bookmarks/data/DistanceAndAzimut");
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
  Java_com_mapswithme_maps_bookmarks_data_Bookmark_getXY(
       JNIEnv * env, jobject thiz, jint cat, jlong bmk)
  {
    return jni::GetNewParcelablePointD(env, getBookmark(cat, bmk)->GetOrg());
  }
}
