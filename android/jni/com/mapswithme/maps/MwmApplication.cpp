#include "com/mapswithme/maps/Framework.hpp"

#include "com/mapswithme/util/crashlytics.h"

#include "com/mapswithme/platform/GuiThread.hpp"
#include "com/mapswithme/platform/Platform.hpp"

#include "com/mapswithme/core/jni_helper.hpp"

extern "C"
{
  // void nativeInitPlatform(String apkPath, String storagePath, String privatePath, String tmpPath,
  // String obbGooglePath, String flavorName, String buildType, boolean isTablet);
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativeInitPlatform(JNIEnv * env, jobject thiz,
                                                             jstring apkPath, jstring storagePath,
                                                             jstring privatePath, jstring tmpPath,
                                                             jstring obbGooglePath,
                                                             jstring flavorName, jstring buildType,
                                                             jboolean isTablet)
  {
    android::Platform::Instance().Initialize(env, thiz, apkPath, storagePath, privatePath, tmpPath,
                                             obbGooglePath, flavorName, buildType, isTablet);
  }

  // static void nativeInitFramework();
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativeInitFramework(JNIEnv * env, jclass clazz)
  {
    if (!g_framework)
      g_framework = std::make_unique<android::Framework>();
  }

  // static void nativeProcessTask(long taskPointer);
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativeProcessTask(JNIEnv * env, jclass clazz, jlong taskPointer)
  {
    android::GuiThread::ProcessTask(taskPointer);
  }

  // static void nativeAddLocalization(String name, String value);
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativeAddLocalization(JNIEnv * env, jclass clazz, jstring name, jstring value)
  {
    g_framework->AddString(jni::ToNativeString(env, name),
                           jni::ToNativeString(env, value));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativeOnTransit(JNIEnv *, jclass, jboolean foreground)
  {
    if (static_cast<bool>(foreground))
      g_framework->NativeFramework()->EnterForeground();
    else
      g_framework->NativeFramework()->EnterBackground();
  }
}
