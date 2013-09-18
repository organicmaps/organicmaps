#include <jni.h>
#include <android/log.h>

#include "Framework.hpp"
#include "../../../base/logging.hpp"
#include "../../../std/shared_ptr.hpp"

namespace
{
  static shared_ptr<yopme::Framework> s_framework;
}

#define FRAMEWORK_CHECK() ASSERT(s_framework != NULL, ())

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
Java_com_mapswithme_yopme_map_MapRenderer_nativeRenderMyPosition(JNIEnv * env, jobject obj,
                                                                 double lat, double lon, double zoom)
{
//  ASSERT(s_framework != NULL, ());
  FRAMEWORK_CHECK();
  return s_framework->ShowMyPosition(lat, lon, zoom);
}

JNIEXPORT bool JNICALL
Java_com_mapswithme_yopme_map_MapRenderer_nativeRenderPoiMap(JNIEnv * env, jobject obj,
                                                            double lat, double lon,
                                                            bool needMyLoc, double myLat, double myLon,
                                                            double zoom)
{
//  ASSERT(s_framework != NULL, ());
  FRAMEWORK_CHECK();
  return s_framework->ShowPoi(lat, lon, needMyLoc, myLat, myLon, zoom);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_yopme_map_MapRenderer_nativeOnMapFileUpdate(JNIEnv * env, jobject thiz)
{
  FRAMEWORK_CHECK();
  s_framework->NativeFramework().RemoveLocalMaps();
  s_framework->NativeFramework().AddLocalMaps();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_yopme_map_MapRenderer_nativeOnKmlFileUpdate(JNIEnv * env, jobject thiz)
{
  FRAMEWORK_CHECK();
  s_framework->NativeFramework().LoadBookmarks();
}

} // extern "C"
