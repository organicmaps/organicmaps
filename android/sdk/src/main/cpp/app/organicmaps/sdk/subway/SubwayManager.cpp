#include <jni.h>
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/platform/AndroidPlatform.hpp"

extern "C"
{
static void TransitSchemeStateChanged(TransitReadManager::TransitSchemeState state,
                                      std::shared_ptr<jobject> const & listener)
{
  JNIEnv * env = jni::GetEnv();
  env->CallVoidMethod(*listener, jni::GetMethodID(env, *listener, "onTransitStateChanged", "(I)V"),
                      static_cast<jint>(state));
}

JNIEXPORT void Java_app_organicmaps_sdk_maplayer_subway_SubwayManager_nativeAddListener(JNIEnv * env, jclass clazz,
                                                                                        jobject listener)
{
  g_framework->SetTransitSchemeListener(
      std::bind(&TransitSchemeStateChanged, std::placeholders::_1, jni::make_global_ref(listener)));
}

JNIEXPORT void Java_app_organicmaps_sdk_maplayer_subway_SubwayManager_nativeRemoveListener(JNIEnv * env, jclass clazz)
{
  g_framework->SetTransitSchemeListener(TransitReadManager::TransitStateChangedFn());
}
}
