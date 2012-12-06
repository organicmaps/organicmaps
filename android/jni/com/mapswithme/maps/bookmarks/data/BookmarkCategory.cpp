#include "../../Framework.hpp"

#include "../../../core/jni_helper.hpp"

namespace {
  ::Framework * frm() { return g_framework->NativeFramework(); }
}

extern "C"
{

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nIsVisible(
      JNIEnv * env, jobject thiz, jint id)
  {
    return frm()->GetBmCategory(id)->IsVisible();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nSetVisibility(
      JNIEnv * env, jobject thiz, jint id, jboolean b)
  {
    return frm()->GetBmCategory(id)->SetVisible(b);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nSetName(
      JNIEnv * env, jobject thiz, jint id, jstring n)
  {
    return frm()->GetBmCategory(id)->SetName(jni::ToNativeString(env, n));
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nGetName(
       JNIEnv * env, jobject thiz, jint id)
  {
    return jni::ToJavaString(env, frm()->GetBmCategory(id)->GetName());
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nGetSize(
       JNIEnv * env, jobject thiz, jint id)
  {
    return frm()->GetBmCategory(id)->GetBookmarksCount();
  }
}
