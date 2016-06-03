#include "private.h"

#include <jni.h>

extern "C"
{
  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_alohalyticsUrl(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(ALOHALYTICS_URL);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_flurryKey(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(FLURRY_KEY);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_myTrackerKey(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(MY_TRACKER_KEY);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_myTargetSlot(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(MY_TARGET_KEY);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_myTargetCheckUrl(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(AD_PERMISION_SERVER_URL);
  }

  JNIEXPORT jlong JNICALL
  Java_com_mapswithme_maps_PrivateVariables_myTargetCheckInterval(JNIEnv * env, jclass clazz)
  {
    return static_cast<jlong>(AD_PERMISION_CHECK_DURATION);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_hockeyAppId(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(HOCKEY_APP_KEY);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_hockeyAppBetaId(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(HOCKEY_APP_BETA_KEY);
  }
}
