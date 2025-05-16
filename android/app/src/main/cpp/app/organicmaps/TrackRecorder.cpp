#include "Framework.hpp"

#include "map/gps_tracker.hpp"

#include "app/organicmaps/core/jni_helper.hpp"

#include <chrono>

extern "C"
{
  JNIEXPORT void JNICALL
  Java_app_organicmaps_location_TrackRecorder_nativeSetEnabled(JNIEnv * env, jclass clazz, jboolean enable)
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

  JNIEXPORT jboolean JNICALL
  Java_app_organicmaps_location_TrackRecorder_nativeIsEnabled(JNIEnv * env, jclass clazz)
  {
    return GpsTracker::Instance().IsEnabled();
  }

  JNIEXPORT void JNICALL
  Java_app_organicmaps_location_TrackRecorder_nativeStartTrackRecording(JNIEnv * env, jclass clazz)
  {
    frm()->StartTrackRecording();
  }

  JNIEXPORT void JNICALL
  Java_app_organicmaps_location_TrackRecorder_nativeStopTrackRecording(JNIEnv * env, jclass clazz)
  {
    frm()->StopTrackRecording();
  }

  JNIEXPORT void JNICALL
  Java_app_organicmaps_location_TrackRecorder_nativeSaveTrackRecordingWithName(JNIEnv * env, jclass clazz, jstring name)
  {
    frm()->SaveTrackRecordingWithName(jni::ToNativeString(env, name));
  }

  JNIEXPORT jboolean JNICALL
  Java_app_organicmaps_location_TrackRecorder_nativeIsTrackRecordingEmpty(JNIEnv * env, jclass clazz)
  {
    return frm()->IsTrackRecordingEmpty();
  }

  JNIEXPORT jboolean JNICALL
  Java_app_organicmaps_location_TrackRecorder_nativeIsTrackRecordingEnabled(JNIEnv * env, jclass clazz)
  {
    return frm()->IsTrackRecordingEnabled();
  }
}
