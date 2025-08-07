#include <jni.h>

#include "drape_frontend/gui/drape_gui.hpp"
#include "drape_frontend/gui/speed_limit/speed_limit.hpp"

extern "C"
{
JNIEXPORT void JNICALL Java_app_organicmaps_sdk_widget_gui_SpeedLimit_nativeSetEnabled(JNIEnv *, jclass,
                                                                                       jboolean enabled)
{
  gui::DrapeGui::GetSpeedLimit().SetEnabled(enabled);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_widget_gui_SpeedLimit_nativeSetRadius(JNIEnv *, jclass, jfloat radius)
{
  gui::DrapeGui::GetSpeedLimit().SetSize(radius);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_widget_gui_SpeedLimit_nativeSetPosition(JNIEnv *, jclass, jfloat x,
                                                                                        jfloat y)
{
  gui::DrapeGui::GetSpeedLimit().SetPosition({x, y});
}
}
