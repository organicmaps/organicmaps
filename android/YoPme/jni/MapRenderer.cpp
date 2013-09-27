#include <jni.h>
#include <android/log.h>

#include "Framework.hpp"
#include "../../../base/logging.hpp"
#include "../../../std/shared_ptr.hpp"

namespace
{
  static shared_ptr<yopme::Framework> s_framework;
}

// @TODO refactor and remove that
void InitNVEvent(JavaVM * jvm) {}

extern "C"
{

JNIEXPORT void JNICALL
Java_com_mapswithme_yopme_map_MapRenderer_nativeCreateFramework(JNIEnv * env, jobject obj, int width, int height)
{
  s_framework.reset(new yopme::Framework(width, height));
}

JNIEXPORT bool JNICALL
Java_com_mapswithme_yopme_map_MapRenderer_nativeRenderMap(JNIEnv * env, jobject obj,
                                             jdouble  vpLat,       jdouble vpLon,  jdouble zoom,
                                             jboolean hasPoi,      jdouble poiLat, jdouble poiLon,
                                             jboolean hasLocation, jdouble myLat,  jdouble myLon)
{
  ASSERT(s_framework != NULL, ());
  return s_framework->ShowMap(vpLat, vpLon, zoom, hasPoi, poiLat, poiLon, hasLocation, myLat, myLon);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_yopme_map_MapRenderer_nativeOnMapFileUpdate(JNIEnv * env, jobject thiz)
{
  ASSERT(s_framework != NULL, ());
  s_framework->OnMapFileUpdate();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_yopme_map_MapRenderer_nativeOnKmlFileUpdate(JNIEnv * env, jobject thiz)
{
  ASSERT(s_framework != NULL, ());
  s_framework->OnKmlFileUpdate();
}

} // extern "C"
