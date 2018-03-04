#include "com/mapswithme/core/jni_helper.hpp"

namespace network_policy
{
bool GetNetworkPolicyStatus(JNIEnv * env, jobject obj)
{
  static jmethodID const networkPolicyCanUseMethod =
      jni::GetMethodID(env, obj, "сanUseNetwork", "()Z");
  return env->CallBooleanMethod(obj, networkPolicyCanUseMethod);
}

bool GetCurrentNetworkUsageStatus(JNIEnv * env)
{
  static jclass const clazz = jni::GetGlobalClassRef(env, "com/mapswithme/util/NetworkPolicy");
  static jmethodID const method =
    jni::GetStaticMethodID(env, clazz, "getCurrentNetworkUsageStatus", "()Z");
  return env->CallStaticBooleanMethod(clazz, method);
}
}  // namespace network_policy
