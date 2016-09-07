#include "search/displayed_categories.hpp"

#include "../core/jni_helper.hpp"

extern "C"
{
JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_search_DisplayedCategories_nativeGetKeys(JNIEnv * env, jclass clazz)
{
  return jni::ToJavaStringArray(env, search::DisplayedCategories::GetKeys());
}
}  // extern "C"
