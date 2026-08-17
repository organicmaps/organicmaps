#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/core/jni_java_methods.hpp"

#include "search/result.hpp"

extern "C"
{
JNIEXPORT void Java_app_organicmaps_sdk_search_SearchRecents_nativeGetList(JNIEnv * env, jclass, jobject result)
{
  using namespace jni;

  auto const & items = g_framework->NativeFramework()->GetSearchAPI().GetLastSearchQueries();
  if (items.empty())
    return;

  auto const listAddMethod = ListBuilder::Instance(env).m_add;

  for (auto const & item : items)
  {
    TScopedLocalRef str(env, ToJavaString(env, item.second));
    env->CallBooleanMethod(result, listAddMethod, str.get());
  }
}

JNIEXPORT void Java_app_organicmaps_sdk_search_SearchRecents_nativeAdd(JNIEnv * env, jclass, jstring locale,
                                                                       jstring query)
{
  search::QuerySaver::SearchRequest const sr(jni::ToNativeString(env, locale), jni::ToNativeString(env, query));
  g_framework->NativeFramework()->GetSearchAPI().SaveSearchQuery(sr);
}

JNIEXPORT void Java_app_organicmaps_sdk_search_SearchRecents_nativeClear(JNIEnv * env, jclass)
{
  g_framework->NativeFramework()->GetSearchAPI().ClearSearchHistory();
}
}
