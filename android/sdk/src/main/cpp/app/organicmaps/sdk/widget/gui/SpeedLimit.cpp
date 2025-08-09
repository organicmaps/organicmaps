#include <jni.h>

#include "drape_frontend/gui/speed_limit/speed_limit.hpp"
#include "drape_frontend/gui/drape_gui.hpp"

extern "C"
{
JNIEXPORT void JNICALL Java_app_organicmaps_sdk_widget_gui_SpeedLimit_nativeSetEnabled(JNIEnv *, jclass,
                                                                                       jboolean enabled)
{
  gui::DrapeGui::GetSpeedLimitHelper().SetEnabled(enabled);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_widget_gui_SpeedLimit_nativeSetRadius(JNIEnv *, jclass, jfloat radius)
{
  gui::DrapeGui::GetSpeedLimitHelper().SetSize(radius);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_widget_gui_SpeedLimit_nativeSetPosition(JNIEnv *, jclass, jfloat x,
                                                                                        jfloat y)
{
  gui::DrapeGui::GetSpeedLimitHelper().SetPosition({x, y});
}
}
