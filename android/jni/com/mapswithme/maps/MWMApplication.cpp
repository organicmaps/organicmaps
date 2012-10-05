/*
 * MWMService.cpp
 *
 *  Created on: May 11, 2012
 *      Author: siarheirachytski
 */

#include "Framework.hpp"

#include "../core/jni_helper.hpp"

#include "../platform/Platform.hpp"
#include "../../../../../platform/settings.hpp"
#include "../../../../../map/information_display.hpp"
#include "../../../../../map/location_state.hpp"

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMApplication_nativeInit(JNIEnv * env,
                                                    jobject thiz,
                                                    jstring apkPath,
                                                    jstring storagePath,
                                                    jstring tmpPath,
                                                    jstring extTmpPath,
                                                    jboolean isPro)
  {
    android::Platform::Instance().Initialize(env,
                                             apkPath,
                                             storagePath,
                                             tmpPath,
                                             extTmpPath,
                                             isPro);

    if (!g_framework)
      g_framework = new android::Framework();
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MWMApplication_nativeIsBenchmarking(JNIEnv * env,
                                                               jobject thiz)
  {
    return static_cast<jboolean>(g_framework->NativeFramework()->IsBenchmarking());
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MWMApplication_nativeShouldShowFacebookDialog(JNIEnv * env,
                                                                         jobject thiz)
  {
    return static_cast<jboolean>(g_framework->NativeFramework()->ShouldShowFacebookDialog());
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMApplication_nativeSubmitFacebookDialogResult(JNIEnv * env,
                                                                   jobject thiz,
                                                                   jint result)
  {
    g_framework->NativeFramework()->SaveFacebookDialogResult(static_cast<int>(result));
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MWMApplication_nativeGetBoolean(JNIEnv * env,
                                                           jobject thiz,
                                                           jstring name,
                                                           jboolean defaultVal)
  {
    bool val = defaultVal;
    Settings::Get(jni::ToNativeString(env, name), val);
    return val;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMApplication_nativeSetBoolean(JNIEnv * env,
                                                           jobject thiz,
                                                           jstring name,
                                                           jboolean val)
  {
    bool flag = val;
    (void)Settings::Set(jni::ToNativeString(env, name), flag);
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MWMApplication_nativeIsFollowingCompass(JNIEnv * env,
                                                                   jobject thiz)
  {
    location::ECompassProcessMode compassMode = g_framework->NativeFramework()->GetInformationDisplay().locationState()->CompassProcessMode();
    return compassMode == location::ECompassFollow;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMApplication_nativeStartCompassFollowing(JNIEnv * env,
                                                                      jobject thiz)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    if (!ls->IsCentered())
      ls->AnimateToPositionAndEnqueueFollowing();
    else
      ls->StartCompassFollowing();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMApplication_nativeStopCompassFollowing(JNIEnv * env,
                                                                     jobject thiz)
  {
    g_framework->NativeFramework()->GetInformationDisplay().locationState()->StopCompassFollowing();
  }

  void CompassStatusChanged(int mode, shared_ptr<jobject> const & obj)
  {
    JNIEnv * env = jni::GetEnv();
    jmethodID methodID = jni::GetJavaMethodID(env, *obj.get(), "OnCompassStatusChanged", "(I)V");
    jint val = static_cast<jint>(mode);
    env->CallVoidMethod(*obj.get(), methodID, val);
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_MWMApplication_nativeAddCompassStatusListener(JNIEnv * env, jobject thiz, jobject obj)
  {
    location::State::TCompassStatusListener fn = bind(&CompassStatusChanged, _1, jni::make_global_ref(obj));
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    return ls->AddCompassStatusListener(fn);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMApplication_nativeRemoveCompassStatusListener(JNIEnv * env, jobject thiz, jint slotID)
  {
    shared_ptr<location::State> ls = g_framework->NativeFramework()->GetInformationDisplay().locationState();
    ls->RemoveCompassStatusListener(slotID);
  }
}
