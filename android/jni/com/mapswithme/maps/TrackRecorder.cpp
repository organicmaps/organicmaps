#include "Framework.hpp"
#include "../platform/Platform.hpp"

#include "std/chrono.hpp"

namespace
{

::Framework * frm()
{
  return g_framework->NativeFramework();
}

} // namespace

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_location_TrackRecorder_nativeSetEnabled(JNIEnv * env, jclass clazz, jboolean enable)
  {
    frm()->EnableGpsTracking(enable);
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_location_TrackRecorder_nativeIsEnabled(JNIEnv * env, jclass clazz)
  {
    return frm()->IsGpsTrackingEnabled();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_location_TrackRecorder_nativeSetDuration(JNIEnv * env, jclass clazz, jint durationHours)
  {
    frm()->SetGpsTrackingDuration(hours(durationHours));
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_location_TrackRecorder_nativeGetDuration(JNIEnv * env, jclass clazz)
  {
    return frm()->GetGpsTrackingDuration().count();
  }
}
