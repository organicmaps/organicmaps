#pragma once

#include <jni.h>

#include "geometry/point2d.hpp"

#include "std/string.hpp"
#include "std/shared_ptr.hpp"

// cache MapIndex jclass
extern jclass g_indexClazz;

namespace jni
{
//  jclass FindClass(char const * name);

  /// @name Some examples of signature:
  /// "()V" - void function returning void;
  /// "(Ljava/lang/String;)V" - String function returning void;
  /// "com/mapswithme/maps/MapStorage$Index" - class MapStorage.Index
  //@{
  jmethodID GetJavaMethodID(JNIEnv * env, jobject obj, char const * fn, char const * sig);

  //jobject CreateJavaObject(JNIEnv * env, char const * klass, char const * sig, ...);
  //@}

  JNIEnv * GetEnv();
  JavaVM * GetJVM();

  string ToNativeString(JNIEnv * env, jstring str);

  jstring ToJavaString(JNIEnv * env, char const * s);

  inline jstring ToJavaString(JNIEnv * env, string const & s)
  {
    return ToJavaString(env, s.c_str());
  }

  jclass GetStringClass(JNIEnv * env);
  char const * GetStringClassName();

  string DescribeException();

  shared_ptr<jobject> make_global_ref(jobject obj);

  jobject GetNewParcelablePointD(JNIEnv * env, m2::PointD const & point);

  jobject GetNewPoint(JNIEnv * env, m2::PointD const & point);
  jobject GetNewPoint(JNIEnv * env, m2::PointI const & point);

  void DumpDalvikReferenceTables();
}
