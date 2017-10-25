#include "android/jni/com/mapswithme/maps/Framework.hpp"
#include "android/jni/com/mapswithme/core/jni_helper.hpp"

#include "search/displayed_categories.hpp"

extern "C"
{
JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_search_DisplayedCategories_nativeGetKeys(JNIEnv * env, jclass clazz)
{
  ::Framework * fr = g_framework->NativeFramework();
  ASSERT(fr, ());
  search::DisplayedCategories categories = fr->GetDisplayedCategories();
  return jni::ToJavaStringArray(env, categories.GetKeys());
}
}  // extern "C"
