#include "Framework.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "map/gps_tracker.hpp"

#include <chrono>

extern "C"
{
JNIEXPORT void Java_app_organicmaps_sdk_location_TrackRecorder_nativeSetEnabled(JNIEnv * env, jclass clazz,
                                                                                jboolean enable)
{
  GpsTracker::Instance().SetEnabled(enable);
  Framework * f = frm();
  if (f == nullptr)
    return;
  if (enable)
    f->ConnectToGpsTracker();
  else
    f->DisconnectFromGpsTracker();
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_location_TrackRecorder_nativeIsEnabled(JNIEnv * env, jclass clazz)
{
  return GpsTracker::Instance().IsEnabled();
}

JNIEXPORT void Java_app_organicmaps_sdk_location_TrackRecorder_nativeStartTrackRecording(JNIEnv * env, jclass clazz)
{
  frm()->StartTrackRecording();
}

JNIEXPORT void Java_app_organicmaps_sdk_location_TrackRecorder_nativeSetTrackRecordingStatsListener(
    JNIEnv * env, jclass clazz, jobject updateListener)
{
  if (updateListener == nullptr)
  {
    frm()->SetTrackRecordingUpdateHandler(nullptr);
    return;
  }
  ASSERT(frm()->IsTrackRecordingEnabled(), ());
  static jmethodID const cId = jni::GetConstructorID(env, g_trackStatisticsClazz, "(DDDDII)V");

  frm()->SetTrackRecordingUpdateHandler(
      [listener = jni::make_global_ref(updateListener)](TrackStatistics const & trackStats)
  {
    JNIEnv * env = jni::GetEnvSafe();
    jobject stats =
        env->NewObject(g_trackStatisticsClazz, cId, trackStats.m_length, trackStats.m_duration, trackStats.m_ascent,
                       trackStats.m_descent, trackStats.m_minElevation, static_cast<jint>(trackStats.m_maxElevation));

    jmethodID onUpdateFn = jni::GetMethodID(env, *listener, "onTrackRecordingUpdate",
                                            "(Lapp/organicmaps/sdk/bookmarks/data/TrackStatistics;)V");
    env->CallVoidMethod(*listener, onUpdateFn, stats);
    jni::HandleJavaException(env);
  });
}

JNIEXPORT jobject Java_app_organicmaps_sdk_location_TrackRecorder_nativeGetElevationInfo(JNIEnv * env, jclass clazz)
{
  ASSERT(frm()->IsTrackRecordingEnabled(), ("Track recording is not started"));
  auto const & elevationInfo = frm()->GetTrackRecordingElevationInfo();
  if (elevationInfo.GetSize() == 0)
    return nullptr;
  return ToJavaElevationInfo(env, elevationInfo);
}

JNIEXPORT void Java_app_organicmaps_sdk_location_TrackRecorder_nativeStopTrackRecording(JNIEnv * env, jclass clazz)
{
  frm()->StopTrackRecording();
}

JNIEXPORT void Java_app_organicmaps_sdk_location_TrackRecorder_nativeSaveTrackRecordingWithName(JNIEnv * env,
                                                                                                jclass clazz,
                                                                                                jstring name)
{
  frm()->SaveTrackRecordingWithName(jni::ToNativeString(env, name));
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_location_TrackRecorder_nativeIsTrackRecordingEmpty(JNIEnv * env,
                                                                                               jclass clazz)
{
  return frm()->IsTrackRecordingEmpty();
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_location_TrackRecorder_nativeIsTrackRecordingEnabled(JNIEnv * env,
                                                                                                 jclass clazz)
{
  return frm()->IsTrackRecordingEnabled();
}
}  // namespace
