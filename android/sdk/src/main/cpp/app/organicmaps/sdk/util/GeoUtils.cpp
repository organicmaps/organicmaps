#include "android/sdk/src/main/cpp/app/organicmaps/sdk/core/jni_helper.hpp"

#include "geometry/mercator.hpp"

extern "C"
{
JNIEXPORT jobject JNICALL
  Java_app_organicmaps_sdk_util_GeoUtils_nativeToLatLon(
  JNIEnv * env, jobject thiz, jdouble mercX, jdouble mercY)
{
  auto const mercPoint = m2::PointD(static_cast<double>(mercX), static_cast<double>(mercY));
  auto const latLon = mercator::ToLatLon(mercPoint);
  auto const latlonPoint = m2::PointD(latLon.m_lat, latLon.m_lon);

  return jni::GetNewParcelablePointD(env, latlonPoint);
}
}
