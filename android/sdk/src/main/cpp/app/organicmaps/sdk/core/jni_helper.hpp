#pragma once

#include <jni.h>

#include "ScopedLocalRef.hpp"

#include "geometry/point2d.hpp"

#include "base/buffer_vector.hpp"
#include "base/logging.hpp"

#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

extern jclass g_mapObjectClazz;
extern jclass g_bookmarkClazz;
extern jclass g_trackClazz;
extern jclass g_trackStatisticsClazz;
extern jclass g_httpClientClazz;
extern jclass g_httpParamsClazz;
extern jclass g_platformSocketClazz;
extern jclass g_utilsClazz;
extern jclass g_loggerClazz;
extern jclass g_keyValueClazz;
extern jclass g_networkPolicyClazz;
extern jclass g_elevationInfoClazz;
extern jclass g_uiThreadClazz;

namespace jni
{
JNIEnv * GetEnv();
JavaVM * GetJVM();

jmethodID GetMethodID(JNIEnv * env, jobject obj, char const * name, char const * signature);
jmethodID GetStaticMethodID(JNIEnv * env, jclass clazz, char const * name, char const * signature);
jmethodID GetConstructorID(JNIEnv * env, jclass clazz, char const * signature);
jfieldID GetStaticFieldID(JNIEnv * env, jclass clazz, char const * name, char const * signature);

// Result value should be DeleteGlobalRef`ed by caller
jclass GetGlobalClassRef(JNIEnv * env, char const * s);

std::string ToNativeString(JNIEnv * env, jstring str);
// Converts UTF-8 array to native UTF-8 string. Result differs from simple GetStringUTFChars call for characters greater
// than U+10000, since jni uses modified UTF (MUTF-8) for strings.
std::string ToNativeString(JNIEnv * env, jbyteArray const & utfBytes);
jstring ToJavaString(JNIEnv * env, char const * s);
inline jstring ToJavaString(JNIEnv * env, std::string const & s)
{
  return ToJavaString(env, s.c_str());
}
inline jstring ToJavaString(JNIEnv * env, std::string_view sv)
{
  /// @todo Make conversion without a temporary some day.
  return ToJavaString(env, std::string(sv).c_str());
}

// Remove after dropping Android 5 and 6 support.
jstring ToJavaStringWithSupplementalCharsFix(JNIEnv * env, std::string const & s);

jclass GetStringClass(JNIEnv * env);
char const * GetStringClassName();

std::string DescribeException();
bool HandleJavaException(JNIEnv * env);
base::LogLevel GetLogLevelForException(JNIEnv * env, jthrowable const & e);

std::shared_ptr<jobject> make_global_ref(jobject obj);
std::shared_ptr<jobject> make_global_ref_safe(jobject obj);

using TScopedLocalRef = ScopedLocalRef<jobject>;
using TScopedLocalClassRef = ScopedLocalRef<jclass>;
using TScopedLocalObjectArrayRef = ScopedLocalRef<jobjectArray>;
using TScopedLocalIntArrayRef = ScopedLocalRef<jintArray>;
using TScopedLocalByteArrayRef = ScopedLocalRef<jbyteArray>;
using TScopedLocalLongArrayRef = ScopedLocalRef<jlongArray>;

// jlong and its unsigned counterpart only: ToNativeLongVector() reads the ids through a jlong *, and aliasing
// allows that for exactly this signed/unsigned pair. Same trait on ToJavaLongArray() for a matching element type.
template <typename T>
inline constexpr bool kIsJavaLong = std::is_same_v<T, jlong> || std::is_same_v<T, std::make_unsigned_t<jlong>>;

// Copies a range of 64-bit integer ids into a new Java long[]. Goes through a buffer because the source is often
// a std::set, which is not contiguous; for the id counts we deal with the extra copy is not worth an overload.
template <typename Range>
jlongArray ToJavaLongArray(JNIEnv * env, Range const & values)
{
  static_assert(kIsJavaLong<typename Range::value_type>);

  std::vector<jlong> const buffer(std::begin(values), std::end(values));
  auto const size = static_cast<jsize>(buffer.size());
  jlongArray result = env->NewLongArray(size);
  // CheckJNI rejects the null buffer of an empty vector even for a zero-length region.
  if (size != 0)
    env->SetLongArrayRegion(result, 0, size, buffer.data());
  return result;
}

// Copies a Java long[] into a contiguous container of 64-bit integer ids.
template <typename Vector>
Vector ToNativeLongVector(JNIEnv * env, jlongArray values)
{
  static_assert(kIsJavaLong<typename Vector::value_type>);
  static_assert(std::contiguous_iterator<typename Vector::iterator>);

  auto const size = env->GetArrayLength(values);
  Vector result(static_cast<size_t>(size));
  // CheckJNI rejects the null buffer of an empty vector even for a zero-length region.
  if (size != 0)
    env->GetLongArrayRegion(values, 0, size, reinterpret_cast<jlong *>(result.data()));
  return result;
}

jobject GetNewParcelablePointD(JNIEnv * env, m2::PointD const & point);

jobject GetNewPoint(JNIEnv * env, m2::PointD const & point);
jobject GetNewPoint(JNIEnv * env, m2::PointI const & point);

template <typename TIt, typename TToJavaFn>
jobjectArray ToJavaArray(JNIEnv * env, jclass clazz, TIt begin, TIt end, size_t const size, TToJavaFn && toJavaFn)
{
  jobjectArray jArray = env->NewObjectArray((jint)size, clazz, 0);
  jint i = 0;
  for (auto it = begin; it != end; ++it)
  {
    TScopedLocalRef jItem(env, toJavaFn(env, *it));
    env->SetObjectArrayElement(jArray, i, jItem.get());
    ++i;
  }

  return jArray;
}

template <typename TContainer, typename TToJavaFn>
jobjectArray ToJavaArray(JNIEnv * env, jclass clazz, TContainer const & src, TToJavaFn && toJavaFn)
{
  return ToJavaArray(env, clazz, std::begin(src), std::end(src), src.size(), std::forward<TToJavaFn>(toJavaFn));
}

template <typename Cont>
jobjectArray ToJavaStringArray(JNIEnv * env, Cont const & src)
{
  return ToJavaArray(env, GetStringClass(env), src,
                     [](JNIEnv * env, std::string const & item) { return ToJavaString(env, item.c_str()); });
}

void DumpDalvikReferenceTables();

jobject ToKeyValue(JNIEnv * env, std::pair<std::string, std::string> src);

template <typename Container>
jobjectArray ToKeyValueArray(JNIEnv * env, Container const & src)
{
  return jni::ToJavaArray(env, g_keyValueClazz, src, &ToKeyValue);
}

std::pair<std::string, std::string> ToNativeKeyValue(JNIEnv * env, jobject pairOfStrings);

template <typename OutputIt>
void ToNativekeyValueContainer(JNIEnv * env, jobjectArray src, OutputIt it)
{
  jint const length = env->GetArrayLength(src);
  for (jint i = 0; i < length; ++i)
  {
    jni::ScopedLocalRef<jobject> const arrayItem(env, env->GetObjectArrayElement(src, i));

    *it = ToNativeKeyValue(env, arrayItem.get());
    ++it;
  }
}
}  // namespace jni
