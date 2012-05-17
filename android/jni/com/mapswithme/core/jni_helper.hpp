#pragma once

#include <jni.h>

#include "../../../../../std/string.hpp"
#include "../../../../../std/shared_ptr.hpp"

namespace jni
{
  // Some examples of sig:
  // "()V" - void function returning void;
  // "(Ljava/lang/String;)V" - String function returning void;
  jmethodID GetJavaMethodID(JNIEnv * env, jobject obj, char const * fn, char const * sig);
  string ToString(jstring str);
  JNIEnv * GetEnv();
  JavaVM * GetJVM();

  shared_ptr<jobject> make_global_ref(jobject obj);
}
