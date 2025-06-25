#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "search/displayed_categories.hpp"

extern "C"
{
JNIEXPORT jobjectArray JNICALL Java_app_organicmaps_sdk_search_DisplayedCategories_nativeGetKeys(JNIEnv * env, jclass)
{
  ::Framework * fr = g_framework->NativeFramework();
  ASSERT(fr, ());
  search::DisplayedCategories const & categories = fr->GetDisplayedCategories();
  return jni::ToJavaStringArray(env, categories.GetKeys());
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_search_DisplayedCategories_nativeIsLangSupported(
        JNIEnv * env, jclass, jstring langCode)
{
  return search::DisplayedCategories::IsLanguageSupported(jni::ToNativeString(env, langCode));
}
}  // extern "C"
