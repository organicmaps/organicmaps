#pragma once

#include <jni.h>

#include "../../../../../platform/location_service.hpp"
#include "../../../../../std/scoped_ptr.hpp"
#include "../jni/jni_method.hpp"

namespace android
{
  class LocationService
  {
  private:

    jobject m_javaObserver;
    location::LocationObserver & m_observer;

    scoped_ptr<jni::Method> m_onLocationChanged;
    scoped_ptr<jni::Method> m_onStatusChanged;

  public:

    LocationService(location::LocationObserver & locationObserver,
                    jobject observer);

    void Start(bool doChangeStatus);
    void Stop(bool doChangeStatus);
    void Disable();
    void OnLocationUpdate(location::GpsInfo const & info);
    void OnLocationStatusChanged(int status);
  };
}

extern android::LocationService * g_locationService;
