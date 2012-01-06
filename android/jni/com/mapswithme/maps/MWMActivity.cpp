#include <jni.h>

#include "../core/logging.hpp"

#include "Framework.hpp"
#include "../platform/Platform.hpp"
#include "../../../../../platform/settings.hpp"
#include "../../../nv_event/nv_event.hpp"
#include "../jni/jni_thread.hpp"
#include "../../../../../base/logging.hpp"

JavaVM * g_jvm;

extern "C"
{
  JNIEXPORT jint JNICALL
  JNI_OnLoad(JavaVM * jvm, void * reserved)
  {
    jni::InitSystemLog();
    jni::InitAssertLog();

    LOG(LINFO, ("logging services initialized"));

    jni::SetCurrentJVM(jvm);
    InitNVEvent(jvm);
    g_jvm = jvm;
    LOG(LDEBUG, ("JNI_OnLoad"));
    return JNI_VERSION_1_6;
  }

  JNIEXPORT void JNICALL
  JNI_OnUnload(JavaVM * vm, void * reserved)
  {
    delete g_framework;
    jni::SetCurrentJVM(0);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeInit(JNIEnv * env, jobject thiz,
      jstring apkPath, jstring storagePath, jstring tmpPath, jstring extTmpPath, jstring settingsPath)
  {
    if (!g_framework)
    {
      android::Platform::Instance().Initialize(env, apkPath, storagePath,
          tmpPath, extTmpPath, settingsPath);
      g_framework = new android::Framework(g_jvm);
    }
  }

////////////////////////////////////////////////////////////////////////////////////////////

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeLocationStatusChanged(JNIEnv * env, jobject thiz,
      int status)
  {
    g_framework->OnLocationStatusChanged(status);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeLocationUpdated(JNIEnv * env, jobject thiz,
      jlong time, jdouble lat, jdouble lon, jfloat accuracy)
  {
    g_framework->OnLocationUpdated(time, lat, lon, accuracy);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeCompassUpdated(JNIEnv * env, jobject thiz,
      jlong time, jdouble magneticNorth, jdouble trueNorth, jfloat accuracy)
  {
    g_framework->OnCompassUpdated(time, magneticNorth, trueNorth, accuracy);
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MWMActivity_hasMeasurementSystem(JNIEnv * env, jobject thiz)
  {
    Settings::Units u;
    return Settings::Get("Units", u);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_setMeasurementSystem(JNIEnv * env, jobject thiz, jint systemIdx)
  {
    Settings::Units u = (Settings::Units)systemIdx;
    Settings::Set("Units", u);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_setupMeasurementSystem(JNIEnv * env, jobject thiz)
  {
    g_framework->SetupMeasurementSystem();
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_MWMActivity_getMeasurementSystem(JNIEnv * env, jobject thiz)
  {
    Settings::Units u = Settings::Metric;
    Settings::Get("Units", u);
    return u;
  }

  //////////////////////////////////////////////////////////////////////////////

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeStorageConnected(JNIEnv * env, jobject thiz)
  {
    android::Platform::Instance().OnExternalStorageStatusChanged(true);
    g_framework->AddLocalMaps();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeStorageDisconnected(JNIEnv * env, jobject thiz)
  {
    android::Platform::Instance().OnExternalStorageStatusChanged(false);
    g_framework->RemoveLocalMaps();
  }

} // extern "C"
