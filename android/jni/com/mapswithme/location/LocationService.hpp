#pragma once

#include <jni.h>

#include "../../../../../platform/location_service.hpp"
#include "../../../../../std/scoped_ptr.hpp"
#include "../jni/jni_method.hpp"

namespace android
{
  class LocationService : public location::LocationService
  {
  private:

    jobject m_javaObserver;

    scoped_ptr<jni::Method> m_onLocationChanged;
    scoped_ptr<jni::Method> m_onStatusChanged;

  public:

    LocationService(location::LocationObserver & locationObserver,
                    jobject observer);

    void Start();
    void Stop();
    void Disable();
    void OnLocationUpdate(location::GpsInfo const & info);
  };
}

extern android::LocationService * g_locationService;
