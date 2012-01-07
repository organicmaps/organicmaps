#include <jni.h>

#include "../../../../../base/assert.hpp"
#include "../../../../../base/logging.hpp"

#include "../../../../../std/string.hpp"

/// Defined and initialized in MWMActivity.cpp
extern JavaVM * g_jvm;

#define DEFAULT_LANG "en"

/// This function is called from native c++ code
string GetAndroidSystemLanguage()
{
  JNIEnv * env = 0;
  if (!g_jvm || g_jvm->AttachCurrentThread(&env, 0) || !env)
  {
    LOG(LWARNING, ("Can't attach thread"));
    return DEFAULT_LANG;
  }

  jclass localeClass = env->FindClass("java/util/Locale");
  ASSERT(localeClass, ("Can't find java class java/util/Locale"));

  jmethodID localeGetDefaultId = env->GetStaticMethodID(localeClass, "getDefault", "()Ljava/util/Locale;");
  ASSERT(localeGetDefaultId, ("Can't find static java/util/Locale.getDefault() method"));

  jobject localeInstance = env->CallStaticObjectMethod(localeClass, localeGetDefaultId);
  ASSERT(localeInstance, ("Locale.getDefault() returned NULL"));

  jmethodID localeGetLanguageId = env->GetMethodID(localeClass, "getLanguage", "()Ljava/lang/String;");
  ASSERT(localeGetLanguageId, ("Can't find java/util/Locale.getLanguage() method"));

  jstring langString = (jstring)env->CallObjectMethod(localeInstance, localeGetLanguageId);
  ASSERT(langString, ("Locale.getLanguage() returned NULL"));

  char const * langUtf8 = env->GetStringUTFChars(langString, 0);
  string result(DEFAULT_LANG);
  if (langUtf8 != 0)
  {
    result = langUtf8;
    env->ReleaseStringUTFChars(langString, langUtf8);
  }
  g_jvm->DetachCurrentThread();
  return result;
}
