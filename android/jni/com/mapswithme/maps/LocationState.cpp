#include "Framework.hpp"

#include "../core/jni_helper.hpp"

#include "../platform/Platform.hpp"

extern "C"
{
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_LocationState_nativeSwitchToNextMode(JNIEnv * env, jobject thiz)
{
  g_framework->NativeFramework()->SwitchMyPositionNextMode();
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_LocationState_nativeGetMode(JNIEnv * env, jobject thiz)
{
  return g_framework->GetMyPositionMode();
}

void LocationStateModeChanged(location::EMyPositionMode mode, bool routingActive, shared_ptr<jobject> const & obj)
{
  g_framework->SetMyPositionMode(mode);

  JNIEnv * env = jni::GetEnv();
  env->CallVoidMethod(*obj, jni::GetMethodID(env, *obj.get(), "onMyPositionModeChangedCallback", "(IZ)V"),
                      static_cast<jint>(mode), static_cast<jboolean>(routingActive));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_LocationState_nativeSetListener(JNIEnv * env, jobject thiz, jobject obj)
{
  g_framework->SetMyPositionModeListener(bind(&LocationStateModeChanged, _1, _2, jni::make_global_ref(obj)));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_LocationState_nativeRemoveListener(JNIEnv * env, jobject thiz, jint slotID)
{
  g_framework->SetMyPositionModeListener(location::TMyPositionModeChanged());
}
} // extern "C"
