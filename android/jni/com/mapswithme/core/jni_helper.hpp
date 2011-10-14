#pragma once

#include <jni.h>

namespace jni
{
  // Some examples of sig:
  // "()V" - void function returning void;
  // "(Ljava/lang/String;)V" - String function returning void;

  jmethodID GetJavaMethodID(JNIEnv * env, jobject obj,
                            char const * fn, char const * sig);
  /* Example of usage:
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_callbackFromJNI(JNIEnv * env, jobject thiz)
  {
    LOG("Enter callbackFromJNI");

    env->CallVoidMethod(thiz,
        jni::GetJavaMethodID(env, thiz, "callbackVoid", "()V"));
    env->CallVoidMethod(
        thiz,
        jni::GetJavaMethodID(env, thiz, "callbackString",
            "(Ljava/lang/String;)V"), env->NewStringUTF("Pass string from JNI."));

    LOG("Leave callbackFromJNI");
  }
  */
}
