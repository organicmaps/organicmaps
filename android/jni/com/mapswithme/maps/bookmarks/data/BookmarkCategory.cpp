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
    frm()->GetBmCategory(id)->SetVisible(b);
    frm()->GetBmCategory(id)->SaveToKMLFile();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nSetName(
      JNIEnv * env, jobject thiz, jint id, jstring n)
  {
    frm()->GetBmCategory(id)->SetName(jni::ToNativeString(env, n));
    frm()->GetBmCategory(id)->SaveToKMLFile();
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
