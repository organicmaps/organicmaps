#include "Framework.hpp"

#include "search/result.hpp"

#include "android/jni/com/mapswithme/core/jni_helper.hpp"

using TSearchRequest = search::QuerySaver::TSearchRequest;

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchRecents_nativeGetList(JNIEnv * env, jclass thiz, jobject result)
  {
    auto const & items = g_framework->NativeFramework()->GetLastSearchQueries();
    if (items.empty())
      return;

    static jclass const pairClass = jni::GetGlobalClassRef(env, "android/util/Pair");
    static jmethodID const pairCtor = jni::GetConstructorID(env, pairClass, "(Ljava/lang/Object;Ljava/lang/Object;)V");
    static jmethodID const listAddMethod = jni::GetMethodID(env, result, "add", "(Ljava/lang/Object;)Z");

    for (TSearchRequest const & item : items)
    {
      jni::TScopedLocalRef locale(env, jni::ToJavaString(env, item.first.c_str()));
      jni::TScopedLocalRef query(env, jni::ToJavaString(env, item.second.c_str()));
      jni::TScopedLocalRef pair(env, env->NewObject(pairClass, pairCtor, locale.get(), query.get()));
      ASSERT(pair.get(), (jni::DescribeException()));

      env->CallBooleanMethod(result, listAddMethod, pair.get());
    }
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchRecents_nativeAdd(JNIEnv * env, jclass thiz, jstring locale, jstring query)
  {
    TSearchRequest const sr(jni::ToNativeString(env, locale), jni::ToNativeString(env, query));
    g_framework->NativeFramework()->SaveSearchQuery(sr);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchRecents_nativeClear(JNIEnv * env, jclass thiz)
  {
    g_framework->NativeFramework()->ClearSearchHistory();
  }
}
