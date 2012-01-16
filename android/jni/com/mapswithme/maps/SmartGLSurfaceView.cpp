#include <jni.h>

#include "Framework.hpp"

extern "C"
{

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_SmartGLSurfaceView_nativeZoom(JNIEnv * env, jobject thiz,
    jint mode, jfloat x1, jfloat y1, jfloat x2, jfloat y2)
{
  android::GetFramework().Zoom(mode, x1, y1, x2, y2);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_SmartGLSurfaceView_nativeMove(JNIEnv * env, jobject thiz,
    jint mode, jfloat x, jfloat y)
{
  android::GetFramework().Move(mode, x, y);
}

}
