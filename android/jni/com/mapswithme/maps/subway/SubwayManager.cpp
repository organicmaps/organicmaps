#include <jni.h>
#include <android/jni/com/mapswithme/maps/Framework.hpp>
#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/platform/Platform.hpp"

extern "C"
{
static void TransitSchemeStateChanged(TransitReadManager::TransitSchemeState state,
                                      std::shared_ptr<jobject> const & listener)
{
  JNIEnv * env = jni::GetEnv();
  env->CallVoidMethod(*listener,
                      jni::GetMethodID(env, *listener, "onTransitStateChanged", "(I)V"),
                      static_cast<jint>(state));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_maplayer_subway_SubwayManager_nativeAddListener(JNIEnv *env, jclass clazz, jobject listener)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  g_framework->SetTransitSchemeListener(std::bind(&TransitSchemeStateChanged,
                                                  std::placeholders::_1,
                                                  jni::make_global_ref(listener)));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_maplayer_subway_SubwayManager_nativeRemoveListener(JNIEnv * env, jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  g_framework->SetTransitSchemeListener(TransitReadManager::TransitStateChangedFn());
}
}
