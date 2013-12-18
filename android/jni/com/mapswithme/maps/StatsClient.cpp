#include "../../../../../stats/client/stats_client.hpp"
#include "../core/jni_helper.hpp"

extern "C"
{
  static stats::Client * g_client = 0;

  stats::Client * NativeStat()
  {
    if (!g_client)
      g_client = new stats::Client();
    return g_client;
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_util_StatsClient_trackSearchQuery(JNIEnv * env, jclass clazz, jstring query)
  {
    return NativeStat()->Search(jni::ToNativeString(env, query));
  }
}
