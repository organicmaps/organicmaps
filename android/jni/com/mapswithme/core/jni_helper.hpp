#pragma once

#include <jni.h>

#include "../../../../../std/string.hpp"
#include "../../../../../std/shared_ptr.hpp"

namespace jni
{
  /// @name Some examples of signature:
  /// "()V" - void function returning void;
  /// "(Ljava/lang/String;)V" - String function returning void;
  /// "com/mapswithme/maps/MapStorage$Index" - class MapStorage.Index
  //@{
  jmethodID GetJavaMethodID(JNIEnv * env, jobject obj, char const * fn, char const * sig);

  //jobject CreateJavaObject(JNIEnv * env, char const * klass, char const * sig, ...);
  //@}

  string ToNativeString(jstring str);
  jstring ToJavaString(string const & s);
  JNIEnv * GetEnv();
  JavaVM * GetJVM();

  string DescribeException();

  shared_ptr<jobject> make_global_ref(jobject obj);
}
