#include "Framework.hpp"
#include "map/gps_tracker.hpp"

extern "C"
{
  JNIEXPORT void JNICALL
  Java_app_organicmaps_location_LocationHelper_nativeOnLocationError(JNIEnv * env, jclass clazz, int errorCode)
  {
    ASSERT(g_framework, ());
    g_framework->OnLocationError(errorCode);
  }

  JNIEXPORT void JNICALL
  Java_app_organicmaps_location_LocationHelper_nativeLocationUpdated(JNIEnv * env, jclass clazz, jlong time,
                                                                         jdouble lat, jdouble lon, jfloat accuracy,
                                                                         jdouble altitude, jfloat speed, jfloat bearing)
  {
    ASSERT(g_framework, ());
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
      info.m_speedMpS = speed;

    g_framework->OnLocationUpdated(info);
    GpsTracker::Instance().OnLocationUpdated(info);
  }
}
