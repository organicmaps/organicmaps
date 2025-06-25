#include "android/sdk/src/main/cpp/app/organicmaps/sdk/core/jni_helper.hpp"
#include "platform/preferred_languages.hpp"

extern "C"
{
JNIEXPORT jstring JNICALL
Java_app_organicmaps_sdk_util_Language_nativeNormalize(JNIEnv *env, jclass type, jstring lang)
{
  std::string locale = languages::Normalize(jni::ToNativeString(env, lang));
  return jni::ToJavaString(env, locale);
}
}
