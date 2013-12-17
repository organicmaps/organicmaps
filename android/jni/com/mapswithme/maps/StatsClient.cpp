#include "../../../../../stats/client/stats_client.hpp"
#include "../core/jni_helper.hpp"

extern "C"
{
  static stats::Client * m_client = 0;

  stats::Client * NativeStat()
  {
    if (!m_client)
      m_client = new stats::Client();
    return m_client;
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_util_StatsClient_trackSearchQuery(JNIEnv * env, jobject thiz, jstring query)
  {
    NativeStat()->Search(jni::ToNativeString(env, query));
    return true;
  }
}
