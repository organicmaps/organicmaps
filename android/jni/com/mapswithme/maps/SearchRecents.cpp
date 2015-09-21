#include "Framework.hpp"

#include "search/result.hpp"

#include "../core/jni_helper.hpp"

using TSearchRequest = search::QuerySaver::TSearchRequest;

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchRecents_nativeGetList(JNIEnv * env, jclass thiz, jobject result)
  {
    auto const & items = g_framework->NativeFramework()->GetLastSearchQueries();
    if (items.empty())
      return;

    static jclass const pairClass = static_cast<jclass>(env->NewGlobalRef(env->FindClass("android/util/Pair")));
    static jmethodID const pairCtor = env->GetMethodID(pairClass, "<init>", "(Ljava/lang/Object;Ljava/lang/Object;)V");
    static jmethodID const listAddMethod = env->GetMethodID(env->GetObjectClass(result), "add", "(Ljava/lang/Object;)Z");

    for (TSearchRequest const & item : items)
    {
      jstring locale = jni::ToJavaString(env, item.first.c_str());
      jstring query = jni::ToJavaString(env, item.second.c_str());
      jobject pair = env->NewObject(pairClass, pairCtor, locale, query);
      ASSERT(pair, (jni::DescribeException()));

      env->CallBooleanMethod(result, listAddMethod, pair);

      env->DeleteLocalRef(locale);
      env->DeleteLocalRef(query);
      env->DeleteLocalRef(pair);
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
