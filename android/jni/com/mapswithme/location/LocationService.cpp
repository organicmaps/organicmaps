/*
 * LocationService.cpp
 *
 *  Created on: Oct 13, 2011
 *      Author: siarheirachytski
 */

#include <jni.h>
#include "../maps/Framework.hpp"

///////////////////////////////////////////////////////////////////////////////////
// LocationService
///////////////////////////////////////////////////////////////////////////////////

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_location_LocationService_nativeEnableLocationService(JNIEnv * env, jobject thiz,
      jboolean enable)
  {
    g_framework->EnableLocation(enable);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_location_LocationService_nativeLocationChanged(JNIEnv * env, jobject thiz,
      jlong time, jdouble lat, jdouble lon, jfloat accuracy)
  {
    g_framework->UpdateLocation(time, lat, lon, accuracy);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_location_LocationService_nativeCompassChanged(JNIEnv * env, jobject thiz,
      jlong time, jdouble magneticNorth, jdouble trueNorth, jfloat accuracy)
  {
    g_framework->UpdateCompass(time, magneticNorth, trueNorth, accuracy);
  }
}
