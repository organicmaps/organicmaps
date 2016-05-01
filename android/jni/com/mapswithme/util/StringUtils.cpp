#include "com/mapswithme/core/jni_helper.hpp"

#include "indexer/search_string_utils.hpp"

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
    string substr = jni::ToNativeString(env, jSubstr);
    int const length = env->GetArrayLength(src);
    vector<string> filtered;
    filtered.reserve(length);
    for (int i = 0; i < length; i++)
    {
      string str = jni::ToNativeString(env, (jstring) env->GetObjectArrayElement(src, i));
      if (search::ContainsNormalized(str, substr))
        filtered.push_back(str);
    }

    return jni::ToJavaStringArray(env, filtered);
  }
} // extern "C"
