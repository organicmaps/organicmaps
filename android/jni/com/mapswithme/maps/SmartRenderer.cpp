#include <jni.h>

#include "Framework.hpp"

extern "C"
{

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_SmartRenderer_nativeLoadResources(JNIEnv * env, jobject thiz)
{
  g_framework->InitRenderPolicy();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_SmartRenderer_nativeUnloadResources(JNIEnv * env, jobject thiz)
{
  g_framework->DeleteRenderPolicy();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_SmartRenderer_nativeDrawFrame(JNIEnv * env, jobject thiz)
{
  g_framework->DrawFrame();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_SmartRenderer_nativeResize(JNIEnv * env, jobject thiz, jint width, jint height)
{
  g_framework->Resize(width, height);
}

}
