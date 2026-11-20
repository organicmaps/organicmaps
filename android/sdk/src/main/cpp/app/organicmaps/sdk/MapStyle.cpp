#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/Framework.hpp"

extern "C"
{
JNIEXPORT jint Java_app_organicmaps_sdk_MapStyle_nativeGet(JNIEnv *, jclass)
{
  return g_framework->GetMapStyle();
}

JNIEXPORT void Java_app_organicmaps_sdk_MapStyle_nativeSetForCurrentMode(JNIEnv *, jclass, jboolean dark,
                                                                         jboolean vehicleStyle,
                                                                         jboolean isRendererActive)
{
  g_framework->SetMapStyleForCurrentMode(dark, vehicleStyle, isRendererActive);
}
}
