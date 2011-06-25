#include "logging.h"
#include "platform.h"

#include <string.h>
#include <jni.h>


extern "C"
{
  ///////////////////////////////////////////////////////////////////////////////////
  // MWMActivity
  ///////////////////////////////////////////////////////////////////////////////////

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeInit(JNIEnv * env, jobject activity, jstring path)
  {
    GetAndroidPlatform().Initialize(env, activity, path);
  }

  ///////////////////////////////////////////////////////////////////////////////////
  // MainGLView
  ///////////////////////////////////////////////////////////////////////////////////

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MainGLView_nativeInit(JNIEnv * env, jobject thiz)
  {
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_GesturesProcessor_nativeMove(JNIEnv * env,
      jobject thiz, jint mode, jdouble x, jdouble y)
  {
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_GesturesProcessor_nativeZoom(JNIEnv * env,
      jobject thiz, jint mode, jdouble x1, jdouble y1, jdouble x2, jdouble y2)
  {
  }

  ///////////////////////////////////////////////////////////////////////////////////
  // MainRenderer
  ///////////////////////////////////////////////////////////////////////////////////

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MainRenderer_nativeInit(JNIEnv * env, jobject thiz)
  {
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MainRenderer_nativeResize(JNIEnv * env, jobject thiz, jint w, jint h)
  {
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MainRenderer_nativeDraw(JNIEnv * env, jobject thiz)
  {
  }
}
