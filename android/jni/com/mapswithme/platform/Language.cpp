#include "../core/jni_helper.hpp"

#include "../../../../../base/assert.hpp"
#include "../../../../../base/logging.hpp"

#include "../../../../../std/string.hpp"


static char const * DEFAULT_LANG = "en";

/// This function is called from native c++ code
string GetAndroidSystemLanguage()
{
  JNIEnv * env = jni::GetEnv();
  if (!env)
  {
    LOG(LWARNING, ("Can't get JNIEnv"));
    return DEFAULT_LANG;
  }

  jclass localeClass = env->FindClass("java/util/Locale");
  ASSERT(localeClass, ());

  jmethodID localeGetDefaultId = env->GetStaticMethodID(localeClass, "getDefault", "()Ljava/util/Locale;");
  ASSERT(localeGetDefaultId, ());

  jobject localeInstance = env->CallStaticObjectMethod(localeClass, localeGetDefaultId);
  ASSERT(localeInstance, ());

  jmethodID localeGetLanguageId = env->GetMethodID(localeClass, "getLanguage", "()Ljava/lang/String;");
  ASSERT(localeGetLanguageId, ());

  jstring langString = (jstring)env->CallObjectMethod(localeInstance, localeGetLanguageId);
  ASSERT(langString, ());

  string res = jni::ToNativeString(env, langString);
  if (res.empty())
    res = DEFAULT_LANG;

  LOG(LDEBUG, ("System language:", res));
  return res;
}
