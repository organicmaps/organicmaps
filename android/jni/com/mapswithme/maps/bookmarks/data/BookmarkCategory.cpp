#include "../../Framework.hpp"

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
    return getBmCategory(id)->GetBookmarksCount();
  }
}
