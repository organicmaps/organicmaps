#include "Framework.hpp"

#include "app/organicmaps/sdk/bookmarks/data/ElevationInfo.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "map/gps_tracker.hpp"

#include <chrono>

extern "C"
{
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
    // The handler is a platform::SafeCallback, so it always runs on the attached GUI thread.
    JNIEnv * env = jni::GetEnv();
    jobject stats =
        env->NewObject(g_trackStatisticsClazz, cId, trackStats.m_length, trackStats.m_duration, trackStats.m_ascent,
                       trackStats.m_descent, trackStats.m_minElevation, static_cast<jint>(trackStats.m_maxElevation));

    jmethodID onUpdateFn = jni::GetMethodID(env, *listener, "onTrackRecordingUpdate",
                                            "(Lapp/organicmaps/sdk/bookmarks/data/TrackStatistics;)V");
    env->CallVoidMethod(*listener, onUpdateFn, stats);
    jni::HandleJavaException(env);
  });
}

JNIEXPORT jobject Java_app_organicmaps_sdk_location_TrackRecorder_nativeGetElevationInfo(JNIEnv * env, jclass clazz,
                                                                                         jobject previous,
                                                                                         jobject cancelled)
{
  static jmethodID const get = jni::GetMethodID(env, cancelled, "get", "()Z");
  auto const isCancelled = [=] { return env->CallBooleanMethod(cancelled, get) == JNI_TRUE; };
  auto const snapshot = GpsTracker::Instance().GetElevationSnapshot(isCancelled);
  if (!snapshot || snapshot->m_revision < 2 || isCancelled())
    return nullptr;
  static jfieldID const epoch = env->GetFieldID(g_elevationInfoClazz, "mRecordingEpoch", "J");
  static jfieldID const revision = env->GetFieldID(g_elevationInfoClazz, "mRecordingRevision", "J");
  if (previous && env->GetLongField(previous, epoch) == static_cast<jlong>(snapshot->m_epoch) &&
      env->GetLongField(previous, revision) == static_cast<jlong>(snapshot->m_revision))
    return previous;
  auto result = ToJavaElevationInfo(env, snapshot->m_elevation, isCancelled);
  if (result)
  {
    env->SetLongField(result, epoch, static_cast<jlong>(snapshot->m_epoch));
    env->SetLongField(result, revision, static_cast<jlong>(snapshot->m_revision));
  }
  return result;
}

JNIEXPORT void Java_app_organicmaps_sdk_location_TrackRecorder_nativeStopTrackRecording(JNIEnv * env, jclass clazz)
{
  frm()->StopTrackRecording();
}

JNIEXPORT void Java_app_organicmaps_sdk_location_TrackRecorder_nativeDiscardTrackRecording(JNIEnv * env, jclass clazz)
{
  frm()->DiscardTrackRecording();
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
