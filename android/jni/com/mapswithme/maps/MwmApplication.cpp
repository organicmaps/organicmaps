#include "Framework.hpp"

#include "../core/jni_helper.hpp"

#include "../platform/Platform.hpp"

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativeInitPlatform(JNIEnv * env, jobject thiz, jstring apkPath, jstring storagePath, jstring tmpPath,
                                                             jstring obbGooglePath, jstring flavorName, jstring buildType, jboolean isYota, jboolean isTablet)
  {
    android::Platform::Instance().Initialize(env, thiz, apkPath, storagePath, tmpPath, obbGooglePath, flavorName, buildType, isYota, isTablet);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativeInitFramework(JNIEnv * env, jclass clazz)
  {
    if (!g_framework)
      g_framework = new android::Framework();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativeProcessFunctor(JNIEnv * env, jclass clazz, jlong functorPointer)
  {
    android::Platform::Instance().ProcessFunctor(functorPointer);
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativeHasFreeSpace(JNIEnv * env, jclass clazz, jlong size)
  {
    return android::Platform::Instance().HasAvailableSpaceForWriting(size);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MwmApplication_nativeAddLocalization(JNIEnv * env, jclass clazz, jstring name, jstring value)
  {
    g_framework->AddString(jni::ToNativeString(env, name),
                           jni::ToNativeString(env, value));
  }
}
