#pragma once

#include <jni.h>

#include "ScopedLocalRef.hpp"

#include "geometry/point2d.hpp"

#include "std/string.hpp"
#include "std/shared_ptr.hpp"

extern jclass g_mapObjectClazz;
extern jclass g_bookmarkClazz;

namespace jni
{
JNIEnv * GetEnv();
JavaVM * GetJVM();

jmethodID GetMethodID(JNIEnv * env, jobject obj, char const * fn, char const * sig);
jmethodID GetConstructorID(JNIEnv * env, jclass clazz, char const * sig);

// Result value should be DeleteGlobalRef`ed by caller
jclass GetGlobalClassRef(JNIEnv * env, char const * s);

string ToNativeString(JNIEnv * env, jstring str);
// Converts UTF-8 array to native UTF-8 string. Result differs from simple GetStringUTFChars call for characters greater than U+10000,
// since jni uses modified UTF (MUTF-8) for strings.
string ToNativeString(JNIEnv * env, jbyteArray const & utfBytes);
jstring ToJavaString(JNIEnv * env, char const * s);
inline jstring ToJavaString(JNIEnv * env, string const & s)
{
  return ToJavaString(env, s.c_str());
}

jclass GetStringClass(JNIEnv * env);
char const * GetStringClassName();

string DescribeException();

shared_ptr<jobject> make_global_ref(jobject obj);
using TScopedLocalRef = ScopedLocalRef<jobject>;
using TScopedLocalClassRef = ScopedLocalRef<jclass>;
using TScopedLocalObjectArrayRef = ScopedLocalRef<jobjectArray>;
using TScopedLocalIntArrayRef = ScopedLocalRef<jintArray>;
using TScopedLocalByteArrayRef = ScopedLocalRef<jbyteArray>;

jobject GetNewParcelablePointD(JNIEnv * env, m2::PointD const & point);

jobject GetNewPoint(JNIEnv * env, m2::PointD const & point);
jobject GetNewPoint(JNIEnv * env, m2::PointI const & point);

template<typename TValue, typename TToJavaFn>
jobjectArray ToJavaArray(JNIEnv * env, jclass clazz, vector<TValue> const & src, TToJavaFn && toJavaFn)
{
  size_t const size = src.size();
  jobjectArray jArray = env->NewObjectArray((jint) size, clazz, 0);
  for (size_t i = 0; i < size; i++)
  {
    TScopedLocalRef jItem(env, toJavaFn(env, src[i]));
    env->SetObjectArrayElement(jArray, i, jItem.get());
  }

  return jArray;
}
jobjectArray ToJavaStringArray(JNIEnv * env, vector<string> const & src);

void DumpDalvikReferenceTables();
}
