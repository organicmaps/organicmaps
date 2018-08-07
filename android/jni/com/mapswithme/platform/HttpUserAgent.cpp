#include "com/mapswithme/core/jni_helper.hpp"
#include "platform/http_user_agent.hpp"

namespace platform
{
std::string HttpUserAgent::ExtractAppVersion() const
{
    JNIEnv * env = jni::GetEnv();
    static auto const buildConfigClass = jni::GetGlobalClassRef(env,
                                                                "com/mapswithme/maps/BuildConfig");
    auto const versionNameField = jni::GetStaticFieldID(env, buildConfigClass, "VERSION_NAME",
                                                        "Ljava/lang/String;");
    jni::TScopedLocalRef versionNameResult(env, env->GetStaticObjectField(buildConfigClass,
                                                                          versionNameField));
    return jni::ToNativeString(env, static_cast<jstring>(versionNameResult.get()));
}
}  // platform
