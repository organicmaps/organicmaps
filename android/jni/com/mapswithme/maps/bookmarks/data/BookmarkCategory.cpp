#include "../../Framework.hpp"

#include "../../../core/jni_helper.hpp"

namespace {
  ::Framework * frm() { return g_framework->NativeFramework(); }
}

extern "C"
{

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nIsVisible(
      JNIEnv * env, jobject thiz, jstring id)
  {
    return frm()->GetBmCategory(jni::ToNativeString(env, id))->IsVisible();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nSetVisibility(
      JNIEnv * env, jobject thiz, jstring id, jboolean b)
  {
    return frm()->GetBmCategory(jni::ToNativeString(env, id))->SetVisible(b);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nSetName(
      JNIEnv * env, jobject thiz, jstring id, jstring n)
  {
    return frm()->GetBmCategory(jni::ToNativeString(env, id))->SetName(jni::ToNativeString(env, n));
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nGetName(
       JNIEnv * env, jobject thiz, jint id)
  {
    return jni::ToJavaString(env, frm()->GetBmCategory(id)->GetName());
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_bookmarks_data_BookmarkCategory_nGetSize(
       JNIEnv * env, jobject thiz, jstring name)
  {
    return frm()->GetBmCategory(jni::ToNativeString(env, name))->GetBookmarksCount();
  }
}
