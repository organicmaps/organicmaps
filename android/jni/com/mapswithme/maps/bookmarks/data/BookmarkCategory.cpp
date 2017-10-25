#include "com/mapswithme/maps/Framework.hpp"
#include "com/mapswithme/maps/UserMarkHelper.hpp"
#include "com/mapswithme/core/jni_helper.hpp"

#include "map/place_page_info.hpp"
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
  pCat->SetIsVisible(b);
  pCat->NotifyChanges();
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

// TODO(AlexZ): Get rid of UserMarks completely in UI code.
// TODO(yunikkk): Refactor java code to get all necessary info without Bookmark wrapper, and without hierarchy.
// If bookmark information is needed in the BookmarkManager, it does not relate in any way to Place Page info
// and should be passed separately via simple name string and lat lon to calculate a distance.
JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nativeGetBookmark(
     JNIEnv * env, jobject thiz, jint catId, jint bmkId)
{
  BookmarkCategory * category = getBmCategory(catId);
  place_page::Info info;
  frm()->FillBookmarkInfo(*static_cast<Bookmark const *>(category->GetUserMark(bmkId)),
                          {static_cast<size_t>(bmkId), static_cast<size_t>(catId)}, info);
  return usermark_helper::CreateMapObject(env, info);
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

  std::string formattedLength;
  measurement_utils::FormatDistance(nTrack->GetLengthMeters(), formattedLength);

  dp::Color nColor = nTrack->GetColor(0);

  jint androidColor = shift(nColor.GetAlpha(), 24) +
                      shift(nColor.GetRed(), 16) +
                      shift(nColor.GetGreen(), 8) +
                      nColor.GetBlue();

  return env->NewObject(trackClazz, cId,
                        bmkId, id, jni::ToJavaString(env, nTrack->GetName()),
                        jni::ToJavaString(env, formattedLength), androidColor);
}
} // extern "C"
