#include "Framework.hpp"

#include "../core/jni_helper.hpp"

extern "C"
{
  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_LocationState_getCompassProcessMode(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    return ls->CompassProcessMode();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_setCompassProcessMode(JNIEnv * env, jobject thiz, jint mode)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    ls->SetCompassProcessMode((location::ECompassProcessMode)mode);
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_LocationState_getLocationProcessMode(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    return ls->LocationProcessMode();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_setLocationProcessMode(JNIEnv * env, jobject thiz, jint mode)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    return ls->SetLocationProcessMode((location::ELocationProcessMode)mode);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_startCompassFollowing(JNIEnv * env,
                                                               jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    if (!ls->IsCentered())
      ls->AnimateToPositionAndEnqueueFollowing();
    else
      ls->StartCompassFollowing();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_stopCompassFollowing(JNIEnv * env,
                                                              jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    ls->StopCompassFollowing();
  }

  void CompassStatusChanged(int mode, shared_ptr<jobject> const & obj)
  {
    JNIEnv * env = jni::GetEnv();
    jmethodID methodID = jni::GetJavaMethodID(env, *obj.get(), "OnCompassStatusChanged", "(I)V");
    jint val = static_cast<jint>(mode);
    env->CallVoidMethod(*obj.get(), methodID, val);
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_LocationState_addCompassStatusListener(JNIEnv * env, jobject thiz, jobject obj)
  {
    location::State::TCompassStatusListener fn = bind(&CompassStatusChanged, _1, jni::make_global_ref(obj));
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    return ls->AddCompassStatusListener(fn);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_removeCompassStatusListener(JNIEnv * env, jobject thiz, jint slotID)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    ls->RemoveCompassStatusListener(slotID);
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_LocationState_hasPosition(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    return ls->HasPosition();
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_LocationState_hasCompass(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    return ls->HasCompass();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_turnOff(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    return ls->TurnOff();
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_LocationState_isVisible(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    return ls->isVisible();
  }
}
