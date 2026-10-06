#include "WikiData.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

jobject CreateWikiData(JNIEnv * env, std::string const & article)
{
  static jclass clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/WikiData");
  static jmethodID ctorId = jni::GetConstructorID(env, clazz, "(Ljava/lang/String;)V");

  return env->NewObject(clazz, ctorId, jni::ToJavaString(env, article));
}
