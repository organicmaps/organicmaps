#include "ApiData.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

jobject CreateApiData(JNIEnv * env, std::string const & id, std::string const & url)
{
  static jclass clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/ApiData");
  static jmethodID ctorId = jni::GetConstructorID(env, clazz, "(Ljava/lang/String;Ljava/lang/String;)V");

  return env->NewObject(clazz, ctorId, jni::ToJavaString(env, id), jni::ToJavaString(env, url));
}
