#include "../../Framework.hpp"

#include "platform/measurement_utils.hpp"

#include "../../../core/jni_helper.hpp"

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
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_isVisible(
      JNIEnv * env, jobject thiz, jint id)
  {
    return getBmCategory(id)->IsVisible();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_setVisibility(
      JNIEnv * env, jobject thiz, jint id, jboolean b)
  {
    BookmarkCategory * pCat = getBmCategory(id);
    pCat->SetVisible(b);
    pCat->SaveToKMLFile();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_setName(
      JNIEnv * env, jobject thiz, jint id, jstring n)
  {
    BookmarkCategory * pCat = getBmCategory(id);
    pCat->SetName(jni::ToNativeString(env, n));
    pCat->SaveToKMLFile();
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_getName(
       JNIEnv * env, jobject thiz, jint id)
  {
    return jni::ToJavaString(env, getBmCategory(id)->GetName());
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_getSize(
       JNIEnv * env, jobject thiz, jint id)
  {
    BookmarkCategory * category = getBmCategory(id);
    return category->GetBookmarksCount() + category->GetTracksCount();
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_getBookmarksCount(
       JNIEnv * env, jobject thiz, jint id)
  {
    return getBmCategory(id)->GetBookmarksCount();
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_getTracksCount(
       JNIEnv * env, jobject thiz, jint id)
  {
    return getBmCategory(id)->GetTracksCount();
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_getBookmark(
       JNIEnv * env, jobject thiz, jint id, jint index, jclass bookmarkClazz)
  {
    // Bookmark(int categoryId, int bookmarkId, String name)
    jmethodID static const cId = env->GetMethodID(bookmarkClazz, "<init>", "(IILjava/lang/String;)V");

    BookmarkCategory * category = getBmCategory(id);
    Bookmark const * nBookmark = category->GetBookmark(index);

    ASSERT(nBookmark, ("Bookmark must not be null with index:)", index));

    jobject jBookmark = env->NewObject(bookmarkClazz, cId,
                                id, index, jni::ToJavaString(env, nBookmark->GetName()));

    g_framework->InjectMetadata(env, bookmarkClazz, jBookmark, nBookmark);

    return jBookmark;
  }

  static uint32_t shift(uint32_t v, uint8_t bitCount) { return v << bitCount; }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_getTrack(
       JNIEnv * env, jobject thiz, jint id, jint index, jclass trackClazz)
  {
    // Track(int trackId, int categoryId, String name, String lengthString, int color)
    static jmethodID cId = env->GetMethodID(trackClazz, "<init>",
        "(IILjava/lang/String;Ljava/lang/String;I)V");

    BookmarkCategory * category = getBmCategory(id);
    Track const * nTrack = category->GetTrack(index);

    ASSERT(nTrack, ("Track must not be null with index:)", index));

    string formattedLenght;
    MeasurementUtils::FormatDistance(nTrack->GetLengthMeters(), formattedLenght);

    dp::Color nColor = nTrack->GetMainColor();

    jint androidColor = shift(nColor.GetAlfa(), 24) +
                        shift(nColor.GetRed(), 16) +
                        shift(nColor.GetGreen(), 8) +
                        nColor.GetBlue();

    return env->NewObject(trackClazz, cId,
        index, id, jni::ToJavaString(env, nTrack->GetName()),
        jni::ToJavaString(env, formattedLenght), androidColor);
  }
}
