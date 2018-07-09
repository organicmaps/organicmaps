#include "com/mapswithme/core/jni_helper.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <string>

/// This function is called from native c++ code
std::string GetAndroidSystemLanguage()
{
  static char const * DEFAULT_LANG = "en";

  JNIEnv * env = jni::GetEnv();
  if (!env)
  {
    LOG(LWARNING, ("Can't get JNIEnv"));
    return DEFAULT_LANG;
  }

  static jclass const languageClass = jni::GetGlobalClassRef(env, "com/mapswithme/util/Language");
  static jmethodID const getDefaultLocaleId = jni::GetStaticMethodID(env, languageClass, "getDefaultLocale", "()Ljava/lang/String;");

  jni::TScopedLocalRef localeRef(env, env->CallStaticObjectMethod(languageClass, getDefaultLocaleId));

  std::string res = jni::ToNativeString(env, (jstring) localeRef.get());
  if (res.empty())
    res = DEFAULT_LANG;

  return res;
}
