#include <jni.h>

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/core/ScopedLocalRef.hpp"

#include "platform/localization.hpp"

#include <string>

namespace
{
std::string GetLocalizedStringByUtil(std::string const & methodName, std::string const & str)
{
  JNIEnv * env = jni::GetEnv();
  static auto const getLocalizedString = jni::GetStaticMethodID(
      env, g_utilsClazz, methodName.c_str(), "(Ljava/lang/String;)Ljava/lang/String;");

  jni::TScopedLocalRef strRef(env, jni::ToJavaString(env, str));
  auto localizedString =
      env->CallStaticObjectMethod(g_utilsClazz, getLocalizedString, strRef.get());

  return jni::ToNativeString(env, static_cast<jstring>(localizedString));
}
}  // namespace

namespace platform
{
std::string GetLocalizedTypeName(std::string const & type)
{
  return GetLocalizedStringByUtil("getLocalizedFeatureType", type);
}

std::string GetLocalizedBrandName(std::string const & brand)
{
  return GetLocalizedStringByUtil("getLocalizedBrand", brand);
}
}  // namespace platform
