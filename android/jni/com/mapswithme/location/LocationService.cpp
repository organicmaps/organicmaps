/*
 * LocationService.cpp
 *
 *  Created on: Oct 13, 2011
 *      Author: siarheirachytski
 */

#include <jni.h>
#include "../jni/jni_thread.hpp"
#include "../maps/Framework.hpp"
#include "LocationService.hpp"

android::LocationService * g_locationService = 0;

namespace android
{
  LocationService::LocationService(location::LocationObserver & observer,
                                   jobject javaObserver)
    : m_observer(observer),
      m_javaObserver(javaObserver)
  {
    jclass k = jni::GetCurrentThreadJNIEnv()->GetObjectClass(m_javaObserver);
    m_onLocationChanged.reset(new jni::Method(k, "onLocationChanged", "(JDDF)V"));
    m_onStatusChanged.reset(new jni::Method(k, "onStatusChanged", "(I)V"));
  }

  void LocationService::Start(bool doChangeStatus)
  {
    if (doChangeStatus)
    {
      m_observer.OnLocationStatusChanged(location::EStarted);
      m_onStatusChanged->CallVoid(m_javaObserver, location::EStarted);
    }
  }

  void LocationService::Stop(bool doChangeStatus)
  {
    if (doChangeStatus)
    {
      m_observer.OnLocationStatusChanged(location::EStopped);
      m_onStatusChanged->CallVoid(m_javaObserver, location::EStopped);
    }
  }

  void LocationService::Disable()
  {
    m_observer.OnLocationStatusChanged(location::EDisabledByUser);
    m_onStatusChanged->CallVoid(m_javaObserver, location::EDisabledByUser);
  }

  void LocationService::OnLocationUpdate(location::GpsInfo const & info)
  {
    m_observer.OnGpsUpdated(info);
    m_onLocationChanged->CallVoid(m_javaObserver, info.m_timestamp, info.m_latitude, info.m_longitude, info.m_horizontalAccuracy);
  }

  void LocationService::OnLocationStatusChanged(int status)
  {
    m_observer.OnLocationStatusChanged((location::TLocationStatus)status);
    m_onStatusChanged->CallVoid(m_javaObserver, status);
  }
}

///////////////////////////////////////////////////////////////////////////////////
// LocationService
///////////////////////////////////////////////////////////////////////////////////


extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_location_LocationService_nativeStartUpdate(JNIEnv * env, jobject thiz, jobject observer, jboolean changeStatus)
  {
    g_locationService = new android::LocationService(*g_framework, observer);
    g_locationService->Start(changeStatus);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_location_LocationService_nativeStopUpdate(JNIEnv * env, jobject thiz, jboolean changeStatus)
  {
    g_locationService->Stop(changeStatus);
    delete g_locationService;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_location_LocationService_nativeLocationStatusChanged(JNIEnv * env, jobject thiz, int status)
  {
    g_locationService->OnLocationStatusChanged(status);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_location_LocationService_nativeLocationChanged(JNIEnv * env, jobject thiz,
      jlong time, jdouble lat, jdouble lon, jfloat accuracy)
  {
    location::GpsInfo info;

    info.m_horizontalAccuracy = static_cast<double>(accuracy);
    info.m_latitude = lat;
    info.m_longitude = lon;
    info.m_timestamp = time;
    info.m_source = location::EAndroidNative;

    g_locationService->OnLocationUpdate(info);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_location_LocationService_nativeDisable(JNIEnv * env, jobject thiz)
  {
    g_locationService->Disable();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_location_LocationService_nativeCompassChanged(JNIEnv * env, jobject thiz,
      jlong time, jdouble magneticNorth, jdouble trueNorth, jfloat accuracy)
  {
    g_framework->UpdateCompass(time, magneticNorth, trueNorth, accuracy);
  }
}
