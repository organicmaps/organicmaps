#include "TrackStatistics.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

jobject ToJavaTrackStatistics(JNIEnv * env, TrackStatistics const & trackStats)
{
  static jclass clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/TrackStatistics");
  // clang-format off
  static jmethodID const cId = jni::GetConstructorID(env, clazz,
    "("
    "D"  // m_length
    "D"  // m_duration
    "D"  // m_ascent
    "D"  // m_descent
    "I"  // m_minElevation
    "I"  // m_maxElevation
    ")V"
  );
  return env->NewObject(clazz, cId,
    trackStats.m_length,
    trackStats.m_duration,
    trackStats.m_ascent,
    trackStats.m_descent,
    static_cast<jint>(trackStats.m_minElevation),
    static_cast<jint>(trackStats.m_maxElevation)
  );
  // clang-format on
}
