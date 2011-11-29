/*
 * MWMActivity.cpp
 *
 *  Created on: Oct 13, 2011
 *      Author: siarheirachytski
 */

///////////////////////////////////////////////////////////////////////////////////
// MWMActivity
///////////////////////////////////////////////////////////////////////////////////

#include <jni.h>

#include "../core/logging.hpp"

#include "Framework.hpp"
#include "../platform/Platform.hpp"
#include "../../../nv_event/nv_event.hpp"
#include "../jni/jni_thread.hpp"

JavaVM * g_jvm;

extern "C"
{
  JNIEXPORT jint JNICALL
  JNI_OnLoad(JavaVM * jvm, void * reserved)
  {
    jni::SetCurrentJVM(jvm);
    InitNVEvent(jvm);
    g_jvm = jvm;
    jni::InitSystemLog();
    jni::InitAssertLog();
    LOG(LDEBUG, ("JNI_OnLoad"));
    return JNI_VERSION_1_4;
  }

  JNIEXPORT void JNICALL
  JNI_OnUnload(JavaVM * vm, void * reserved)
  {
    delete g_framework;
    jni::SetCurrentJVM(0);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeInit(JNIEnv * env, jobject thiz, jstring apkPath, jstring storagePath)
  {
    if (!g_framework)
    {
      android::Platform::Instance().Initialize(env, apkPath, storagePath);
      g_framework = new android::Framework(g_jvm);
    }
  }
} // extern "C"




