#include "Framework.hpp"

#include "../core/jni_helper.hpp"

#include "../platform/Platform.hpp"

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_switchToNextMode(JNIEnv * env, jobject thiz)
  {
    g_framework->NativeFramework()->SwitchMyPositionNextMode();
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_LocationState_getLocationStateMode(JNIEnv * env, jobject thiz)
  {
    return g_framework->GetMyPositionMode();
  }

  void LocationStateModeChanged(location::EMyPositionMode mode, shared_ptr<jobject> const & obj)
  {
    JNIEnv * env = jni::GetEnv();
    env->CallVoidMethod(*obj.get(), jni::GetJavaMethodID(env, *obj.get(), "onMyPositionModeChangedCallback", "(I)V"), static_cast<jint>(mode));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_setMyPositionModeListener(JNIEnv * env, jobject thiz, jobject obj)
  {
    g_framework->SetMyPositionModeListener(bind(&LocationStateModeChanged, _1, jni::make_global_ref(obj)));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_removeMyPositionModeListener(JNIEnv * env, jobject thiz, jint slotID)
  {
    g_framework->SetMyPositionModeListener(location::TMyPositionModeChanged());
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_invalidatePosition(JNIEnv * env, jobject thiz)
  {
    g_framework->NativeFramework()->InvalidateMyPosition();
  }
}
