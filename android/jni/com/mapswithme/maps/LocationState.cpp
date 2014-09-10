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

  void CompassStatusChanged(location::State::Mode mode, shared_ptr<jobject> const & obj)
  {
    JNIEnv * env = jni::GetEnv();
    jmethodID methodID = jni::GetJavaMethodID(env, *obj.get(), "OnLocationStateModeChanged", "(I)V");
    jint val = static_cast<jint>(mode);
    env->CallVoidMethod(*obj.get(), methodID, val);
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_LocationState_addLocationStateModeListener(JNIEnv * env, jobject thiz, jobject obj)
  {
    location::State::TStateModeListener fn = bind(&CompassStatusChanged, _1, jni::make_global_ref(obj));
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetLocationState();
    return ls->AddStateModeListener(fn);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_removeLocationStateModeListener(JNIEnv * env, jobject thiz, jint slotID)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetLocationState();
    ls->RemoveStateModeListener(slotID);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_turnOff(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetLocationState();
    return ls->TurnOff();
  }
}
