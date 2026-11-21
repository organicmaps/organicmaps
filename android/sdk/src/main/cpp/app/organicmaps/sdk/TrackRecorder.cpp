#include "Framework.hpp"

#include "app/organicmaps/sdk/bookmarks/data/ElevationInfo.hpp"
#include "app/organicmaps/sdk/bookmarks/data/TrackStatistics.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

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

  frm()->SetTrackRecordingUpdateHandler(
      [listener = jni::make_global_ref(updateListener)](TrackStatistics const & trackStats)
  {
    // The handler is a platform::SafeCallback, so it always runs on the attached GUI thread.
    JNIEnv * env = jni::GetEnv();
    jni::TScopedLocalRef stats(env, ToJavaTrackStatistics(env, trackStats, trackStats.HasGpsElevationProfile()));

    jmethodID onUpdateFn = jni::GetMethodID(env, *listener, "onTrackRecordingUpdate",
                                            "(Lapp/organicmaps/sdk/bookmarks/data/TrackStatistics;)V");
    env->CallVoidMethod(*listener, onUpdateFn, stats.get());
    jni::HandleJavaException(env);
  });
}

JNIEXPORT jobject Java_app_organicmaps_sdk_location_TrackRecorder_nativeGetElevationInfo(JNIEnv * env, jclass clazz)
{
  ASSERT(frm()->IsTrackRecordingEnabled(), ("Track recording is not started"));
  auto const & elevationInfo = frm()->GetTrackRecordingElevationInfo();
  return elevationInfo.IsEmpty() ? nullptr : ToJavaElevationInfo(env, elevationInfo);
}

JNIEXPORT void Java_app_organicmaps_sdk_location_TrackRecorder_nativeStopTrackRecording(JNIEnv * env, jclass clazz)
{
  frm()->StopTrackRecording();
}

JNIEXPORT void Java_app_organicmaps_sdk_location_TrackRecorder_nativeDiscardTrackRecording(JNIEnv * env, jclass clazz)
{
  frm()->DiscardTrackRecording();
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_location_TrackRecorder_nativeCanSaveTrackRecording(JNIEnv * env,
                                                                                               jclass clazz)
{
  return frm()->CanSaveTrackRecording();
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_location_TrackRecorder_nativeSaveTrackRecordingWithName(JNIEnv * env,
                                                                                                    jclass clazz,
                                                                                                    jstring name)
{
  return frm()->SaveTrackRecordingWithName(jni::ToNativeString(env, name));
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_location_TrackRecorder_nativeIsTrackRecordingEnabled(JNIEnv * env,
                                                                                                 jclass clazz)
{
  return frm()->IsTrackRecordingEnabled();
}
}  // namespace
