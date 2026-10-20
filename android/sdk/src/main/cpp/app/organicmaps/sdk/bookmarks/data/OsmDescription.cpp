#include "OsmDescription.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

jobject CreateOsmDescription(JNIEnv * env, std::string const & description)
{
  static jclass clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/OsmDescription");
  static jmethodID ctorId = jni::GetConstructorID(env, clazz, "(Ljava/lang/String;)V");
  return env->NewObject(clazz, ctorId, jni::ToJavaString(env, description));
  ;
}
