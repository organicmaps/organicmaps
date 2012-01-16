#include <jni.h>

#include "../core/jni_helper.hpp"

#include "../../../../../base/assert.hpp"
#include "../../../../../base/logging.hpp"

#include "../../../../../std/string.hpp"

#define DEFAULT_LANG "en"

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

  return result;
}
