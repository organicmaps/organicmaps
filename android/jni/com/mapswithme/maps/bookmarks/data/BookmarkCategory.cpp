#include "com/mapswithme/maps/Framework.hpp"
#include "com/mapswithme/maps/UserMarkHelper.hpp"
#include "com/mapswithme/core/jni_helper.hpp"

#include "platform/measurement_utils.hpp"

namespace
{
::Framework * frm() { return g_framework->NativeFramework(); }

BookmarkCategory * getBmCategory(jint c)
{
  BookmarkCategory * pCat = frm()->GetBmCategory(c);
  ASSERT(pCat, ("Category not found", c));
  return pCat;
}
}

extern "C"
{
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nativeIsVisible(
    JNIEnv * env, jobject thiz, jint id)
{
  return getBmCategory(id)->IsVisible();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nativeSetVisibility(
    JNIEnv * env, jobject thiz, jint id, jboolean b)
{
  BookmarkCategory * pCat = getBmCategory(id);
  {
    BookmarkCategory::Guard guard(*pCat);
    guard.m_controller.SetIsVisible(b);
  }
  pCat->SaveToKMLFile();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nativeSetName(
    JNIEnv * env, jobject thiz, jint id, jstring n)
{
  BookmarkCategory * pCat = getBmCategory(id);
  pCat->SetName(jni::ToNativeString(env, n));
  pCat->SaveToKMLFile();
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nativeGetName(
     JNIEnv * env, jobject thiz, jint id)
{
  return jni::ToJavaString(env, getBmCategory(id)->GetName());
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nativeGetSize(
     JNIEnv * env, jobject thiz, jint id)
{
  BookmarkCategory * category = getBmCategory(id);
  return category->GetUserMarkCount() + category->GetTracksCount();
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nativeGetBookmarksCount(
     JNIEnv * env, jobject thiz, jint id)
{
  return getBmCategory(id)->GetUserMarkCount();
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nativeGetTracksCount(
     JNIEnv * env, jobject thiz, jint id)
{
  return getBmCategory(id)->GetTracksCount();
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nativeGetBookmark(
     JNIEnv * env, jobject thiz, jint id, jint bmkId)
{
  return usermark_helper::CreateMapObject(getBmCategory(id)->GetUserMark(bmkId));
}

static uint32_t shift(uint32_t v, uint8_t bitCount) { return v << bitCount; }

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nativeGetTrack(
      JNIEnv * env, jobject thiz, jint id, jint bmkId, jclass trackClazz)
{
  // Track(int trackId, int categoryId, String name, String lengthString, int color)
  static jmethodID const cId = jni::GetConstructorID(env, trackClazz,
                                  "(IILjava/lang/String;Ljava/lang/String;I)V");

  BookmarkCategory * category = getBmCategory(id);
  Track const * nTrack = category->GetTrack(bmkId);

  ASSERT(nTrack, ("Track must not be null with index:)", bmkId));

  string formattedLenght;
  MeasurementUtils::FormatDistance(nTrack->GetLengthMeters(), formattedLenght);

  dp::Color nColor = nTrack->GetColor(0);

  jint androidColor = shift(nColor.GetAlfa(), 24) +
                      shift(nColor.GetRed(), 16) +
                      shift(nColor.GetGreen(), 8) +
                      nColor.GetBlue();

  return env->NewObject(trackClazz, cId,
                        bmkId, id, jni::ToJavaString(env, nTrack->GetName()),
                        jni::ToJavaString(env, formattedLenght), androidColor);
}
} // extern "C"
