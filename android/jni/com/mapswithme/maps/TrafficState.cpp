#include "Framework.hpp"

#include "../core/jni_helper.hpp"

#include "../platform/Platform.hpp"

extern "C"
{
static void TrafficStateChanged(TrafficManager::TrafficState state, shared_ptr<jobject> const & listener)
{
  JNIEnv * env = jni::GetEnv();
  static jmethodID const = jni::GetMethodID(env, *listener.get(), "onTrafficStateChanged", "(I)V");
  env->CallVoidMethod(*listener, jmethodID, static_cast<jint>(state));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_TrafficState_nativeSetListener(JNIEnv * env, jclass clazz, jobject listener)
{
  g_framework->SetTrafficStateListener(bind(&TrafficStateChanged, _1, jni::make_global_ref(listener)));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_TrafficState_nativeRemoveListener(JNIEnv * env, jclass clazz)
{
  g_framework->SetTrafficStateListener(TrafficManager::TrafficStateChangedFn());
}
} // extern "C"
