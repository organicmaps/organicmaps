#include "logging.h"
#include "android_platform.hpp"
#include "android_framework.hpp"

#include "../../storage/storage.hpp"

#include <string.h>
#include <jni.h>

static AndroidFramework * g_work = 0;
static JavaVM * g_jvm = 0;

extern "C"
{
///////////////////////////////////////////////////////////////////////////////////
// MWMActivity
///////////////////////////////////////////////////////////////////////////////////

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM * jvm, void * reserved)
{
  g_jvm = jvm;
  jni::InitSystemLog();
  jni::InitAssertLog();
  LOG(LDEBUG, ("JNI_OnLoad"));
  return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM * vm, void * reserved)
{
  delete g_work;
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MWMActivity_nativeInit(JNIEnv * env, jobject thiz, jstring apkPath, jstring storagePath)
{
  LOG(LDEBUG, ("Java_com_mapswithme_maps_MWMActivity_nativeInit 1"));
  if (!g_work)
  {
    GetAndroidPlatform().Initialize(env, apkPath, storagePath);
    g_work = new AndroidFramework(g_jvm);
  }

  LOG(LDEBUG, ("Java_com_mapswithme_maps_MWMActivity_nativeInit 2"));
}

///////////////////////////////////////////////////////////////////////////////////
// MainGLView
///////////////////////////////////////////////////////////////////////////////////

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MainGLView_nativeInit(JNIEnv * env, jobject thiz)
{
  ASSERT ( g_work, () );
  g_work->SetParentView(thiz);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_GesturesProcessor_nativeMove(JNIEnv * env,
    jobject thiz, jint mode, jdouble x, jdouble y)
{
  ASSERT ( g_work, () );
  g_work->Move(mode, x, y);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_GesturesProcessor_nativeZoom(JNIEnv * env,
    jobject thiz, jint mode, jdouble x1, jdouble y1, jdouble x2, jdouble y2)
{
  ASSERT ( g_work, () );
  g_work->Zoom(mode, x1, y1, x2, y2);
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

///////////////////////////////////////////////////////////////////////////////////
// DownloadUI
///////////////////////////////////////////////////////////////////////////////////

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_DownloadUI_countriesCount(JNIEnv * env, jobject thiz,
    jint group, jint country, jint region)
{
  return static_cast<jint>(g_work->Storage().CountriesCount(storage::TIndex(group, country, region)));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_DownloadUI_countryName(JNIEnv * env, jobject thiz,
    jint group, jint country, jint region)
{
  string const name = g_work->Storage().CountryName(storage::TIndex(group, country, region));
  return env->NewStringUTF(name.c_str());
}

JNIEXPORT jlong JNICALL
Java_com_mapswithme_maps_DownloadUI_countrySizeInBytes(JNIEnv * env, jobject thiz,
    jint group, jint country, jint region)
{
  storage::TLocalAndRemoteSize const s = g_work->Storage().CountrySizeInBytes(storage::TIndex(group, country, region));
  // lower int contains remote size, and upper - local one
  int64_t mergedSize = s.second;
  mergedSize |= (s.first << 32);
  return mergedSize;
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_DownloadUI_countryStatus(JNIEnv * env, jobject thiz,
    jint group, jint country, jint region)
{
  return static_cast<jint>(g_work->Storage().CountryStatus(storage::TIndex(group, country, region)));
}

} // extern "C"
