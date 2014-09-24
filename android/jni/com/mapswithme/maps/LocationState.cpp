#include "Framework.hpp"

#include "../core/jni_helper.hpp"
#include "../../../../../anim/controller.hpp"

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_switchToNextMode(JNIEnv * env, jobject thiz)
  {
    g_framework->NativeFramework()->GetLocationState()->SwitchToNextMode();
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_LocationState_getLocationStateMode(JNIEnv * env, jobject thiz)
  {
    return g_framework->NativeFramework()->GetLocationState()->GetMode();
  }

  void LocationStateModeChanged(location::State::Mode mode, shared_ptr<jobject> const & obj)
  {
    JNIEnv * env = jni::GetEnv();
    env->CallVoidMethod(*obj.get(), jni::GetJavaMethodID(env, *obj.get(), "onLocationStateModeChangedCallback", "(I)V"), static_cast<jint>(mode));
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_LocationState_addLocationStateModeListener(JNIEnv * env, jobject thiz, jobject obj)
  {
    return g_framework->NativeFramework()->GetLocationState()->AddStateModeListener(bind(&LocationStateModeChanged, _1, jni::make_global_ref(obj)));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_removeLocationStateModeListener(JNIEnv * env, jobject thiz, jint slotID)
  {
    g_framework->NativeFramework()->GetLocationState()->RemoveStateModeListener(slotID);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_turnOff(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetLocationState();
    return ls->TurnOff();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_invalidatePosition(JNIEnv * env, jobject thiz)
  {
    g_framework->NativeFramework()->GetLocationState()->InvalidatePosition();
  }
}
