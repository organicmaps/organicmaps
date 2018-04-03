#include "map/framework_light.hpp"

#include "com/mapswithme/core/jni_helper.hpp"

using namespace lightweight;

extern "C"
{
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_LightFramework_nativeIsAuthenticated(JNIEnv * env, jclass clazz)
{
  Framework const framework(REQUEST_TYPE_USER_AUTH_STATUS);
  return static_cast<jboolean>(framework.Get<REQUEST_TYPE_USER_AUTH_STATUS>());
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_LightFramework_nativeGetNumberUnsentUGC(JNIEnv * env, jclass clazz)
{
  Framework const framework(REQUEST_TYPE_NUMBER_OF_UNSENT_UGC);
  return static_cast<jint>(framework.Get<REQUEST_TYPE_NUMBER_OF_UNSENT_UGC>());
}
}  // extern "C"
