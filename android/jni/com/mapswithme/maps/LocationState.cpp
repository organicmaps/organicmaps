#include "Framework.hpp"

#include "../core/jni_helper.hpp"
#include "../../../../../anim/controller.hpp"

extern "C"
{
  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_LocationState_getCompassProcessMode(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    return ls->GetCompassProcessMode();
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

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_LocationState_isCentered(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    return ls->IsCentered();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_animateToPositionAndEnqueueLocationProcessMode(JNIEnv * env, jobject thiz, jint mode)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    ls->AnimateToPositionAndEnqueueLocationProcessMode(static_cast<location::ELocationProcessMode>(mode));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_stopCompassFollowingAndRotateMap(JNIEnv * env,
                                                                          jobject thiz)
  {
    ::Framework * f = g_framework->NativeFramework();
    shared_ptr<location::State> ls = f->GetInformationDisplay().locationState();

    anim::Controller * animController = f->GetAnimController();
    animController->Lock();

    ls->StopCompassFollowing();

    double startAngle = f->GetNavigator().Screen().GetAngle();
    double endAngle = 0;

    f->GetAnimator().RotateScreen(startAngle, endAngle);

    animController->Unlock();

    f->Invalidate();
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
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetLocationState();
    return ls->AddCompassStatusListener(fn);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_removeCompassStatusListener(JNIEnv * env, jobject thiz, jint slotID)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetLocationState();
    ls->RemoveCompassStatusListener(slotID);
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_LocationState_hasPosition(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetLocationState();
    return ls->HasPosition();
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_LocationState_hasCompass(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetLocationState();
    return ls->HasCompass();
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_LocationState_isFirstPosition(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetLocationState();
    return ls->IsFirstPosition();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_turnOff(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetLocationState();
    return ls->TurnOff();
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_LocationState_isVisible(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetLocationState();
    return ls->isVisible();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_onStartLocation(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetLocationState();
    ls->OnStartLocation();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_LocationState_onStopLocation(JNIEnv * env, jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetLocationState();
    ls->OnStopLocation();
  }
}
