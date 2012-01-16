#pragma once

#include <jni.h>

#include "../../../../../std/string.hpp"

namespace jni
{
  // Some examples of sig:
  // "()V" - void function returning void;
  // "(Ljava/lang/String;)V" - String function returning void;
  jmethodID GetJavaMethodID(JNIEnv * env, jobject obj, char const * fn, char const * sig);
  string GetString(JNIEnv * env, jstring str);

  JNIEnv * GetEnv();
  JavaVM * GetJVM();
}
