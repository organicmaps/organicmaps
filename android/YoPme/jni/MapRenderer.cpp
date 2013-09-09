#include <jni.h>
#include <android/log.h>

#include "Framework.hpp"
#include "../../../base/logging.hpp"


// @TODO refactor and remove that
void InitNVEvent(JavaVM * jvm) {}

extern "C"
{

JNIEXPORT void JNICALL
Java_com_mapswithme_yopme_map_MapRenderer_nativeRenderMap(JNIEnv * env, jobject obj, double lat, double lon, double zoom)
{
  yopme::Framework f(360, 640);
  f.ConfigureNavigator(lat, lon, zoom);
  f.RenderMap();
}

} // extern "C"
