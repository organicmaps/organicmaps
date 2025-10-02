#include "Framework.hpp"

#include "map/gps_tracker.hpp"

#include <chrono>

extern "C"
{
JNIEXPORT void JNICALL Java_app_organicmaps_sdk_location_TrackRecorder_nativeStartTrackRecording(JNIEnv * env,
                                                                                                 jclass clazz)
{
  frm()->StartTrackRecording();
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
}
