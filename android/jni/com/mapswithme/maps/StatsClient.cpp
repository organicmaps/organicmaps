#include "../../../../../stats/client/event_tracker.hpp"
#include "../core/jni_helper.hpp"

extern "C"
{
  static stats::EventTracker * g_nativeTracker = 0;

  stats::EventTracker * NativeTracker()
  {
    if (!g_nativeTracker)
      g_nativeTracker = new stats::EventTracker();
    return g_nativeTracker;
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_util_StatsClient_trackSearchQuery(JNIEnv * env, jclass clazz, jstring query)
  {
    return NativeTracker()->TrackSearch(jni::ToNativeString(env, query));
  }
}
