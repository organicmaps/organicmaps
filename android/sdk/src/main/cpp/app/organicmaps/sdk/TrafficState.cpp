#include "Framework.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/platform/AndroidPlatform.hpp"

extern "C"
{
static void TrafficStateChanged(TrafficManager::TrafficState state, std::shared_ptr<jobject> const & listener)
{
  JNIEnv * env = jni::GetEnv();
  env->CallVoidMethod(*listener, jni::GetMethodID(env, *listener, "onTrafficStateChanged", "(I)V"),
                      static_cast<jint>(state));
}

JNIEXPORT void Java_app_organicmaps_sdk_maplayer_traffic_TrafficState_nativeSetListener(JNIEnv * env, jclass clazz,
                                                                                        jobject listener)
{
  g_framework->SetTrafficStateListener(
      std::bind(&TrafficStateChanged, std::placeholders::_1, jni::make_global_ref(listener)));
}

JNIEXPORT void Java_app_organicmaps_sdk_maplayer_traffic_TrafficState_nativeRemoveListener(JNIEnv * env, jclass clazz)
{
  g_framework->SetTrafficStateListener(TrafficManager::TrafficStateChangedFn());
}

JNIEXPORT void Java_app_organicmaps_sdk_maplayer_traffic_TrafficState_nativeEnable(JNIEnv * env, jclass clazz)
{
  g_framework->EnableTraffic();
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_maplayer_traffic_TrafficState_nativeIsEnabled(JNIEnv * env, jclass clazz)
{
  return static_cast<jboolean>(g_framework->IsTrafficEnabled());
}

JNIEXPORT void Java_app_organicmaps_sdk_maplayer_traffic_TrafficState_nativeDisable(JNIEnv * env, jclass clazz)
{
  g_framework->DisableTraffic();
}
}  // extern "C"
