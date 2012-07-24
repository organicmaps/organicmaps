/*
 * MWMService.cpp
 *
 *  Created on: May 11, 2012
 *      Author: siarheirachytski
 */

#include "Framework.hpp"

#include "../core/jni_helper.hpp"

#include "../platform/Platform.hpp"

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMApplication_nativeInit(JNIEnv * env,
                                                    jobject thiz,
                                                    jstring apkPath,
                                                    jstring storagePath,
                                                    jstring tmpPath,
                                                    jstring extTmpPath,
                                                    jstring settingsPath,
                                                    jboolean isPro)
  {
    android::Platform::Instance().Initialize(env,
                                             apkPath,
                                             storagePath,
                                             tmpPath,
                                             extTmpPath,
                                             settingsPath,
                                             isPro);

    if (!g_framework)
      g_framework = new android::Framework();
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MWMApplication_nativeIsBenchmarking(JNIEnv * env,
                                                               jobject thiz)
  {
    return static_cast<jboolean>(g_framework->NativeFramework()->IsBenchmarking());
  }
  }
}
