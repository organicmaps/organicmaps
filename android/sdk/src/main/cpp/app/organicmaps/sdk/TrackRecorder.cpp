#include "Framework.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "map/gps_tracker.hpp"

#include <chrono>

using namespace jni;

extern "C"
{
std::shared_ptr<jobject> g_onUpdate;
JNIEXPORT void JNICALL Java_app_organicmaps_sdk_location_TrackRecorder_nativeSetEnabled(JNIEnv * env, jclass clazz,
                                                                                        jboolean enable)
{
  GpsTracker::Instance().SetEnabled(enable);
  Framework * const f = frm();
  if (f == nullptr)
    return;
  if (enable)
    f->ConnectToGpsTracker();
  else
    f->DisconnectFromGpsTracker();
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_location_TrackRecorder_nativeIsEnabled(JNIEnv * env, jclass clazz)
{
  return GpsTracker::Instance().IsEnabled();
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_location_TrackRecorder_nativeStartTrackRecording(JNIEnv * env,
                                                                                                 jclass clazz)
{
  frm()->StartTrackRecording();
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_location_TrackRecorder_nativeSetTrackRecordingStatsListener(
    JNIEnv * env, jclass clazz, jobject TrackRecordingListener)
{
  if (!frm()->IsTrackRecordingEnabled())
    return;
  if (TrackRecordingListener == nullptr)
  {
    frm()->SetTrackRecordingUpdateHandler({});
    return;
  }
  static jmethodID const cId = jni::GetConstructorID(env, g_trackStatisticsClazz, "(DDDDII)V");
  g_onUpdate = jni::make_global_ref(TrackRecordingListener);
  frm()->SetTrackRecordingUpdateHandler([env](TrackStatistics const & trackStats)
  {
    jobject stats =
        env->NewObject(g_trackStatisticsClazz, cId, trackStats.m_length, trackStats.m_duration, trackStats.m_ascent,
                       trackStats.m_descent, trackStats.m_minElevation, static_cast<jint>(trackStats.m_maxElevation));
    static jmethodID const g_onTrackRecordingUpdate =
        jni::GetMethodID(env, g_onUpdate.operator*(), "onTrackRecordingUpdate",
                         "(Lapp/organicmaps/sdk/bookmarks/data/TrackStatistics;)V");
    env->CallVoidMethod(g_onUpdate.operator*(), g_onTrackRecordingUpdate, stats);
    jni::HandleJavaException(env);
  });
}

JNIEXPORT jobject JNICALL Java_app_organicmaps_sdk_location_TrackRecorder_nativeGetElevationInfo(JNIEnv * env,
                                                                                                 jclass clazz)
{
  if (!frm()->IsTrackRecordingEnabled())
    return nullptr;
  auto const & elevationInfo = frm()->GetTrackRecordingElevationInfo();
  return usermark_helper::CreateElevationInfo(env, elevationInfo);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_location_TrackRecorder_nativeStopTrackRecording(JNIEnv * env,
                                                                                                jclass clazz)
{
  frm()->StopTrackRecording();
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_location_TrackRecorder_nativeSaveTrackRecordingWithName(JNIEnv * env,
                                                                                                        jclass clazz,
                                                                                                        jstring name)
{
  frm()->SaveTrackRecordingWithName(jni::ToNativeString(env, name));
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_location_TrackRecorder_nativeIsTrackRecordingEmpty(JNIEnv * env,
                                                                                                       jclass clazz)
{
  return frm()->IsTrackRecordingEmpty();
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_location_TrackRecorder_nativeIsTrackRecordingEnabled(JNIEnv * env,
                                                                                                         jclass clazz)
{
  return frm()->IsTrackRecordingEnabled();
}
}  // namespace
