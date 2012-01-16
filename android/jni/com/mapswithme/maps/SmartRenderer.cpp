#include <jni.h>

#include "Framework.hpp"

extern "C"
{

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_SmartRenderer_nativeLoadResources(JNIEnv * env, jobject thiz)
{
  android::GetFramework().InitRenderPolicy();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_SmartRenderer_nativeUnloadResources(JNIEnv * env, jobject thiz)
{
  android::GetFramework().DeleteRenderPolicy();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_SmartRenderer_nativeDrawFrame(JNIEnv * env, jobject thiz)
{
  android::GetFramework().DrawFrame();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_SmartRenderer_nativeResize(JNIEnv * env, jobject thiz, jint width, jint height)
{
  android::GetFramework().Resize(width, height);
}

}
