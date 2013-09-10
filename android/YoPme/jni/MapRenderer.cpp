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
                                                          double lat, double lon, double zoom,
                                                          bool needApiMark)
{
  ASSERT(s_framework != NULL, ());
  return s_framework->ShowRect(lat, lon, zoom, needApiMark);
}

} // extern "C"
