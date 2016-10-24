#pragma once

#include <jni.h>

#include "ScopedLocalRef.hpp"

#include "geometry/point2d.hpp"

#include "std/string.hpp"
#include "std/shared_ptr.hpp"

extern jclass g_mapObjectClazz;
extern jclass g_bookmarkClazz;
extern jclass g_myTrackerClazz;
extern jclass g_httpClientClazz;
extern jclass g_httpParamsClazz;
extern jclass g_socketWrapperClazz;
extern jclass g_utilsClazz;

namespace jni
{
JNIEnv * GetEnv();
JavaVM * GetJVM();

jmethodID GetMethodID(JNIEnv * env, jobject obj, char const * name, char const * signature);
jmethodID GetStaticMethodID(JNIEnv * env, jclass clazz, char const * name, char const * signature);
jmethodID GetConstructorID(JNIEnv * env, jclass clazz, char const * signature);

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

template<typename TIt, typename TToJavaFn>
jobjectArray ToJavaArray(JNIEnv * env, jclass clazz, TIt begin, TIt end, size_t const size, TToJavaFn && toJavaFn)
{
  jobjectArray jArray = env->NewObjectArray((jint) size, clazz, 0);
  size_t i = 0;
  for (auto it = begin; it != end; ++it)
  {
    TScopedLocalRef jItem(env, toJavaFn(env, *it));
    env->SetObjectArrayElement(jArray, i, jItem.get());
    ++i;
  }

  return jArray;
}

template<typename TContainer, typename TToJavaFn>
jobjectArray ToJavaArray(JNIEnv * env, jclass clazz, TContainer const & src, TToJavaFn && toJavaFn)
{
  return ToJavaArray(env, clazz, begin(src), end(src), src.size(), forward<TToJavaFn>(toJavaFn));
}

jobjectArray ToJavaStringArray(JNIEnv * env, vector<string> const & src);

void DumpDalvikReferenceTables();
}  // namespace jni
