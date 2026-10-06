#include "RawData.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

jobject CreateRawData(JNIEnv * env, std::vector<std::string> const & rawData)
{
  static jclass clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/RawData");
  static jmethodID ctorId = jni::GetConstructorID(env, clazz, "([Ljava/lang/String;)V");

  return env->NewObject(clazz, ctorId, jni::ToJavaStringArray(env, rawData));
}
