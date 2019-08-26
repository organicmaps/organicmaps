#include <jni.h>

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/core/ScopedLocalRef.hpp"
#include "com/mapswithme/platform/Platform.hpp"

#include "platform/localization.hpp"

#include <string>

namespace
{
jmethodID GetMethodId(std::string const & methodName)
{
  JNIEnv * env = jni::GetEnv();
  return jni::GetStaticMethodID(env, g_utilsClazz, methodName.c_str(),
                                "(Landroid/content/Context;Ljava/lang/String;)Ljava/lang/String;");
}

std::string GetLocalizedStringByUtil(jmethodID const & methodId, std::string const & str)
{
  JNIEnv * env = jni::GetEnv();

  jni::TScopedLocalRef strRef(env, jni::ToJavaString(env, str));
  auto localizedString = env->CallStaticObjectMethod(g_utilsClazz, methodId,
                                                     android::Platform::Instance().GetContext(),
                                                     strRef.get());

  return jni::ToNativeString(env, static_cast<jstring>(localizedString));
}
}  // namespace

namespace platform
{
std::string GetLocalizedTypeName(std::string const & type)
{
  static auto const methodId = GetMethodId("getLocalizedFeatureType");
  return GetLocalizedStringByUtil(methodId, type);
}

std::string GetLocalizedBrandName(std::string const & brand)
{
  static auto const methodId = GetMethodId("getLocalizedBrand");
  return GetLocalizedStringByUtil(methodId, brand);
}

std::string GetLocalizedString(std::string const & key)
{
  static auto const methodId = GetMethodId("getStringValueByKey");
  return GetLocalizedStringByUtil(methodId, key);
}
}  // namespace platform
