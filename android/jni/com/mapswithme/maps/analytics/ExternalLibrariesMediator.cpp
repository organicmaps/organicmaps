#include "com/mapswithme/maps/Framework.hpp"

#include "com/mapswithme/util/crashlytics.h"

#include "com/mapswithme/platform/GuiThread.hpp"
#include "com/mapswithme/platform/Platform.hpp"

#include "com/mapswithme/core/jni_helper.hpp"

crashlytics_context_t * g_crashlytics;

extern "C"

// @UiThread
// static void nativeInitCrashlytics();
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_analytics_ExternalLibrariesMediator_nativeInitCrashlytics(JNIEnv * env, jclass clazz)
{
  ASSERT(!g_crashlytics, ());
  g_crashlytics = crashlytics_init();
}
