#include "android/jni/com/mapswithme/core/jni_helper.hpp"

#include "indexer/search_string_utils.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

extern "C"
{
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_util_StringUtils_nativeIsHtml(JNIEnv * env, jclass thiz, jstring text)
{
  return strings::IsHTML(jni::ToNativeString(env, text));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_util_StringUtils_nativeContainsNormalized(JNIEnv * env, jclass thiz, jstring str, jstring substr)
{
  return search::ContainsNormalized(jni::ToNativeString(env, str), jni::ToNativeString(env, substr));
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_util_StringUtils_nativeFilterContainsNormalized(JNIEnv * env, jclass thiz, jobjectArray src, jstring jSubstr)
{
  std::string substr = jni::ToNativeString(env, jSubstr);
  int const length = env->GetArrayLength(src);
  std::vector<std::string> filtered;
  filtered.reserve(length);
  for (int i = 0; i < length; i++)
  {
    std::string str = jni::ToNativeString(env, (jstring) env->GetObjectArrayElement(src, i));
    if (search::ContainsNormalized(str, substr))
      filtered.push_back(str);
  }

  return jni::ToJavaStringArray(env, filtered);
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_util_StringUtils_nativeFormatSpeedAndUnits(JNIEnv * env, jclass thiz, jdouble metersPerSecond)
{
  static jclass const pairClass = jni::GetGlobalClassRef(env, "android/util/Pair");
  static jmethodID const pairCtor = jni::GetConstructorID(env, pairClass, "(Ljava/lang/Object;Ljava/lang/Object;)V");

  measurement_utils::Units units;
  if (!settings::Get(settings::kMeasurementUnits, units))
    units = measurement_utils::Units::Metric;
  return env->NewObject(pairClass, pairCtor,
                        jni::ToJavaString(env, measurement_utils::FormatSpeed(metersPerSecond, units)),
                        jni::ToJavaString(env, measurement_utils::FormatSpeedUnits(units)));
}
} // extern "C"
