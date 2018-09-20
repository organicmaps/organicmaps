#include <jni.h>

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/core/ScopedLocalRef.hpp"

#include "platform/localization.hpp"

#include <string>

namespace platform
{
std::string GetLocalizedTypeName(std::string const & type)
{
  JNIEnv * env = jni::GetEnv();
  static auto const getLocalizedFeatureType = jni::GetStaticMethodID(
      env, g_utilsClazz, "getLocalizedFeatureType", "(Ljava/lang/String;)Ljava/lang/String;");

  jni::TScopedLocalRef typeRef(env, jni::ToJavaString(env, type));
  auto localizedFeatureType =
      env->CallStaticObjectMethod(g_utilsClazz, getLocalizedFeatureType, typeRef.get());

  return jni::ToNativeString(env, static_cast<jstring>(localizedFeatureType));
}
}  // namespace platform
