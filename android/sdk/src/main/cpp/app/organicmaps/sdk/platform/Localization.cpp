#include <jni.h>

#include "app/organicmaps/sdk/core/ScopedLocalRef.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/platform/AndroidPlatform.hpp"

#include "platform/localization.hpp"

#include "base/assert.hpp"

#include <memory>
#include <string>

namespace
{
class AndroidStringCollator final : public platform::StringCollator
{
public:
  explicit AndroidStringCollator(std::string const & locale)
  {
    JNIEnv * env = jni::GetEnv();
    static jclass const localeClass = jni::GetGlobalClassRef(env, "java/util/Locale");
    static jmethodID const forLanguageTag =
        jni::GetStaticMethodID(env, localeClass, "forLanguageTag", "(Ljava/lang/String;)Ljava/util/Locale;");
    static jclass const collatorClass = jni::GetGlobalClassRef(env, "java/text/Collator");
    static jmethodID const getInstance =
        jni::GetStaticMethodID(env, collatorClass, "getInstance", "(Ljava/util/Locale;)Ljava/text/Collator;");

    jni::TScopedLocalRef localeTag(env, jni::ToJavaString(env, locale));
    jni::TScopedLocalRef javaLocale(env, env->CallStaticObjectMethod(localeClass, forLanguageTag, localeTag.get()));
    jni::TScopedLocalRef collator(env, env->CallStaticObjectMethod(collatorClass, getInstance, javaLocale.get()));
    CHECK(!jni::HandleJavaException(env), (locale));
    m_collator = jni::make_global_ref(collator.get());
    m_compare = jni::GetMethodID(env, collator.get(), "compare", "(Ljava/lang/String;Ljava/lang/String;)I");
  }

  bool Less(std::string const & lhs, std::string const & rhs) const override
  {
    JNIEnv * env = jni::GetEnv();
    jni::TScopedLocalRef left(env, jni::ToJavaString(env, lhs));
    jni::TScopedLocalRef right(env, jni::ToJavaString(env, rhs));
    auto const result = env->CallIntMethod(*m_collator, m_compare, left.get(), right.get());
    CHECK(!jni::HandleJavaException(env), ());
    return result < 0;
  }

private:
  std::shared_ptr<jobject> m_collator;
  jmethodID m_compare;
};

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
  jobject context = android::Platform::Instance().GetContext();
  jni::TScopedLocalRef localizedStrRef(env, env->CallStaticObjectMethod(g_utilsClazz, methodId, context, strRef.get()));
  return jni::ToNativeString(env, static_cast<jstring>(localizedStrRef.get()));
}
}  // namespace

namespace platform
{
std::unique_ptr<StringCollator> CreateStringCollator(std::string const & locale)
{
  return std::make_unique<AndroidStringCollator>(locale);
}

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

std::string GetCurrencySymbol(std::string const & currencyCode)
{
  JNIEnv * env = jni::GetEnv();
  static auto const methodId =
      jni::GetStaticMethodID(env, g_utilsClazz, "getCurrencySymbol", "(Ljava/lang/String;)Ljava/lang/String;");

  jni::TScopedLocalRef currencyCodeRef(env, jni::ToJavaString(env, currencyCode));
  jni::TScopedLocalRef localizedStrRef(env, env->CallStaticObjectMethod(g_utilsClazz, methodId, currencyCodeRef.get()));
  return jni::ToNativeString(env, static_cast<jstring>(localizedStrRef.get()));
}

std::string GetLocalizedMyPositionBookmarkName()
{
  JNIEnv * env = jni::GetEnv();
  static auto const methodId = jni::GetStaticMethodID(env, g_utilsClazz, "getMyPositionBookmarkName",
                                                      "(Landroid/content/Context;)Ljava/lang/String;");

  jobject context = android::Platform::Instance().GetContext();
  jni::TScopedLocalRef localizedStrRef(env, env->CallStaticObjectMethod(g_utilsClazz, methodId, context));
  return jni::ToNativeString(env, static_cast<jstring>(localizedStrRef.get()));
}
}  // namespace platform
