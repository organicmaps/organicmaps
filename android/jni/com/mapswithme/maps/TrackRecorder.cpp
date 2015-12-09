#include "Framework.hpp"

#include "std/chrono.hpp"

namespace
{

::Framework * frm()
{
  // TODO (trashkalmar): Temp solution until the GPS tracker is uncoupled from the framework.
  return (g_framework ? g_framework->NativeFramework() : nullptr);
}

} // namespace

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_location_TrackRecorder_nativeSetEnabled(JNIEnv * env, jclass clazz, jboolean enable)
  {
    // TODO (trashkalmar): Temp solution until the GPS tracker is uncoupled from the framework.

    ::Framework * const framework = frm();
    if (framework)
    {
      framework->EnableGpsTracking(enable);
      return;
    }

    Settings::Set("GpsTrackingEnabled", static_cast<bool>(enable));
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_location_TrackRecorder_nativeIsEnabled(JNIEnv * env, jclass clazz)
  {
    // TODO (trashkalmar): Temp solution until the GPS tracker is uncoupled from the framework.

    ::Framework * const framework = frm();
    if (framework)
      return framework->IsGpsTrackingEnabled();

    bool res = false;
    Settings::Get("GpsTrackingEnabled", res);
    return res;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_location_TrackRecorder_nativeSetDuration(JNIEnv * env, jclass clazz, jint durationHours)
  {
    frm()->SetGpsTrackingDuration(hours(durationHours));
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_location_TrackRecorder_nativeGetDuration(JNIEnv * env, jclass clazz)
  {
    // TODO (trashkalmar): Temp solution until the GPS tracker is uncoupled from the framework.

    ::Framework * const framework = frm();
    if (framework)
      return framework->GetGpsTrackingDuration().count();

    uint32_t res = 24;
    Settings::Get("GpsTrackingDuration", res);
    return res;
  }
}
