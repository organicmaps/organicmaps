#include "logging.h"
#include "platform.h"
#include "framework.h"

#include <string.h>
#include <jni.h>


AndroidFramework * g_work;

extern "C"
{
  ///////////////////////////////////////////////////////////////////////////////////
  // MWMActivity
  ///////////////////////////////////////////////////////////////////////////////////

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeInit(JNIEnv * env, jobject thiz, jstring path)
  {
    jni::InitSystemLog();

    GetAndroidPlatform().Initialize(env, thiz, path);

    //g_work = new AndroidFramework();
  }

  ///////////////////////////////////////////////////////////////////////////////////
  // MainGLView
  ///////////////////////////////////////////////////////////////////////////////////

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MainGLView_nativeInit(JNIEnv * env, jobject thiz)
  {
    //g_work->SetParentView(env, thiz);
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
