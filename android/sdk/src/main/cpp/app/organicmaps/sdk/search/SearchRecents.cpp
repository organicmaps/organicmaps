#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/core/jni_java_methods.hpp"

#include "search/result.hpp"

using SearchRequest = search::QuerySaver::SearchRequest;

extern "C"
{
JNIEXPORT void JNICALL Java_app_organicmaps_sdk_search_SearchRecents_nativeGetList(JNIEnv * env, jclass, jobject result)
{
  auto const & items = g_framework->NativeFramework()->GetSearchAPI().GetLastSearchQueries();
  if (items.empty())
    return;

  auto const listAddMethod = jni::ListBuilder::Instance(env).m_add;

  for (SearchRequest const & item : items)
  {
    jni::TScopedLocalRef str(env, jni::ToJavaString(env, item.second));
    env->CallBooleanMethod(result, listAddMethod, str.get());
  }
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_search_SearchRecents_nativeAdd(JNIEnv * env, jclass, jstring locale,
                                                                               jstring query)
{
  SearchRequest const sr(jni::ToNativeString(env, locale), jni::ToNativeString(env, query));
  g_framework->NativeFramework()->GetSearchAPI().SaveSearchQuery(sr);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_search_SearchRecents_nativeClear(JNIEnv * env, jclass)
{
  g_framework->NativeFramework()->GetSearchAPI().ClearSearchHistory();
}
}
