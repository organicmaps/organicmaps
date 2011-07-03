#include "logging.h"
#include "platform.h"
#include "framework.h"

#include <string.h>
#include <jni.h>

AndroidFramework * g_work = 0;

extern "C"
{
///////////////////////////////////////////////////////////////////////////////////
// MWMActivity
///////////////////////////////////////////////////////////////////////////////////

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MWMActivity_nativeInit(JNIEnv * env, jobject thiz, jstring apkPath, jstring storagePath)
{
  jni::InitSystemLog();
  jni::InitAssertLog();

  LOG(LDEBUG, ("MWMActivity::Init 1"));
  GetAndroidPlatform().Initialize(env, thiz, apkPath, storagePath);

  LOG(LDEBUG, ("MWMActivity::Init 2"));
  g_work = new AndroidFramework();

  LOG(LDEBUG, ("MWMActivity::Init 3"));
}

///////////////////////////////////////////////////////////////////////////////////
// MainGLView
///////////////////////////////////////////////////////////////////////////////////

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MainGLView_nativeInit(JNIEnv * env, jobject thiz)
{
  ASSERT ( g_work, () );
  g_work->SetParentView(env, thiz);
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
  ASSERT ( g_work, () );
  g_work->InitRenderer();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MainRenderer_nativeResize(JNIEnv * env, jobject thiz, jint w, jint h)
{
  ASSERT ( g_work, () );
  g_work->Resize(w, h);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MainRenderer_nativeDraw(JNIEnv * env, jobject thiz)
{
  ASSERT ( g_work, () );
  g_work->DrawFrame();
}

} // extern "C"
