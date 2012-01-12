#include <jni.h>

#include "Framework.hpp"

extern "C"
{

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_SmartGLSurfaceView_nativeZoom(JNIEnv * env, jobject thiz,
    jint mode, jfloat x1, jfloat y1, jfloat x2, jfloat y2)
{
  g_framework->Zoom(mode, x1, y1, x2, y2);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_SmartGLSurfaceView_nativeMove(JNIEnv * env, jobject thiz,
    jint mode, jfloat x, jfloat y)
{
  g_framework->Move(mode, x, y);
}

}
