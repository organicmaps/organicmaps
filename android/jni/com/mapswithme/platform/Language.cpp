#include "Language.hpp"

#include "android/jni/com/mapswithme/core/jni_helper.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <string>

std::string ReplaceDeprecatedLanguageCode(std::string const & lang)
{
  // in* -> id
  // iw* -> he

  if (strings::StartsWith(lang, "in"))
    return "id";
  else if (strings::StartsWith(lang, "iw"))
    return "he";

  return lang;
}

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

  static jclass const localeClass = jni::GetGlobalClassRef(env, "java/util/Locale");
  static jmethodID const localeGetDefaultId = jni::GetStaticMethodID(env, localeClass, "getDefault", "()Ljava/util/Locale;");
  static jmethodID const localeToStringId = env->GetMethodID(localeClass, "toString", "()Ljava/lang/String;");

  jni::TScopedLocalRef localeInstance(env, env->CallStaticObjectMethod(localeClass, localeGetDefaultId));
  jni::TScopedLocalRef langString(env, env->CallObjectMethod(localeInstance.get(), localeToStringId));

  std::string res = jni::ToNativeString(env, (jstring) langString.get());
  if (res.empty())
    res = DEFAULT_LANG;

  return ReplaceDeprecatedLanguageCode(res);
}
