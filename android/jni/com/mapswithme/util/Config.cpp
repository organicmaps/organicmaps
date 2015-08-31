#include "../core/jni_helper.hpp"
#include "platform/settings.hpp"


extern "C"
{
  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_util_Config_nativeGetBoolean(JNIEnv * env, jclass thiz, jstring name, jboolean defaultVal)
  {
    bool val = defaultVal;
    Settings::Get(jni::ToNativeString(env, name), val);
    return val;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_util_Config_nativeSetBoolean(JNIEnv * env, jclass thiz, jstring name, jboolean val)
  {
    bool flag = val;
    (void)Settings::Set(jni::ToNativeString(env, name), flag);
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_util_Config_nativeGetInt(JNIEnv * env, jclass thiz, jstring name, jint defaultValue)
  {
    jint value;
    if (Settings::Get(jni::ToNativeString(env, name), value))
      return value;

    return defaultValue;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_util_Config_nativeSetInt(JNIEnv * env, jclass thiz, jstring name, jint value)
  {
    (void)Settings::Set(jni::ToNativeString(env, name), value);
  }

  JNIEXPORT jlong JNICALL
  Java_com_mapswithme_util_Config_nativeGetLong(JNIEnv * env, jclass thiz, jstring name, jlong defaultValue)
  {
    jlong value;
    if (Settings::Get(jni::ToNativeString(env, name), value))
      return value;

    return defaultValue;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_util_Config_nativeSetLong(JNIEnv * env, jclass thiz, jstring name, jlong value)
  {
    (void)Settings::Set(jni::ToNativeString(env, name), value);
  }

  JNIEXPORT jdouble JNICALL
  Java_com_mapswithme_util_Config_nativeGetDouble(JNIEnv * env, jclass thiz, jstring name, jdouble defaultValue)
  {
    jdouble value;
    if (Settings::Get(jni::ToNativeString(env, name), value))
      return value;

    return defaultValue;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_util_Config_nativeSetDouble(JNIEnv * env, jclass thiz, jstring name, jdouble value)
  {
    (void)Settings::Set(jni::ToNativeString(env, name), value);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_util_Config_nativeSetString(JNIEnv * env, jclass thiz, jstring name, jstring value)
  {
    (void)Settings::Set(jni::ToNativeString(env, name), jni::ToNativeString(env, value));
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_util_Config_nativeGetString(JNIEnv * env, jclass thiz, jstring name, jstring defaultValue)
  {
    string value;
    if (Settings::Get(jni::ToNativeString(env, name), value))
      return jni::ToJavaString(env, value);

    return defaultValue;
  }
} // extern "C"
