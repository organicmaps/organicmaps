#pragma once

#include <jni.h>

#include "geometry/point2d.hpp"

#include "std/string.hpp"
#include "std/shared_ptr.hpp"

// cache MapIndex jclass
extern jclass g_indexClazz;

namespace jni
{
// TODO yunitsky uncomment and use to load classes from native threads.
// jclass FindClass(char const * name);

  jmethodID GetJavaMethodID(JNIEnv * env, jobject obj, char const * fn, char const * sig);

  // Result value should be DeleteGlobalRef`ed by caller
  jclass GetGlobalClassRef(JNIEnv * env, char const * s);

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
