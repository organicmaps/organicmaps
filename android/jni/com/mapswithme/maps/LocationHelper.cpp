#include "Framework.hpp"
#include "map/gps_tracker.hpp"
#include "platform/file_logging.hpp"

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_location_LocationHelper_nativeOnLocationError(JNIEnv * env, jclass clazz, int errorCode)
  {
    g_framework->OnLocationError(errorCode);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_location_LocationHelper_nativeLocationUpdated(JNIEnv * env, jclass clazz, jlong time,
                                                                         jdouble lat, jdouble lon, jfloat accuracy,
                                                                         jdouble altitude, jfloat speed, jfloat bearing)
  {
    location::GpsInfo info;
    info.m_source = location::EAndroidNative;

    info.m_timestamp = static_cast<double>(time) / 1000.0;
    info.m_latitude = lat;
    info.m_longitude = lon;

    if (accuracy > 0.0)
      info.m_horizontalAccuracy = accuracy;

    if (altitude != 0.0)
    {
      info.m_altitude = altitude;
      info.m_verticalAccuracy = accuracy;
    }

    if (bearing > 0.0)
      info.m_bearing = bearing;

    if (speed > 0.0)
      info.m_speed = speed;

    LOG_MEMORY_INFO();
    if (g_framework)
      g_framework->OnLocationUpdated(info);
    GpsTracker::Instance().OnLocationUpdated(info);
  }

  JNIEXPORT jfloatArray JNICALL
  Java_com_mapswithme_maps_location_LocationHelper_nativeUpdateCompassSensor(JNIEnv * env, jclass clazz, jint ind, jfloatArray arr)
  {
    int const kCoordsCount = 3;

    // Extract coords
    jfloat coords[kCoordsCount];
    env->GetFloatArrayRegion(arr, 0, kCoordsCount, coords);
    g_framework->UpdateCompassSensor(ind, coords);

    // Put coords back to java result array
    jfloatArray ret = (jfloatArray)env->NewFloatArray(kCoordsCount);
    env->SetFloatArrayRegion(ret, 0, kCoordsCount, coords);
    return ret;
  }
}
