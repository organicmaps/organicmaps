#include "private.h"

#include "android/jni/com/mapswithme/core/jni_helper.hpp"

#include <jni.h>

#include <string>

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
  Java_com_mapswithme_maps_PrivateVariables_appsFlyerKey(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(APPSFLYER_KEY);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_myTrackerKey(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(MY_TRACKER_KEY);
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_PrivateVariables_myTargetSlot(JNIEnv * env, jclass clazz)
  {
    return MY_TARGET_KEY;
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_PrivateVariables_myTargetRbSlot(JNIEnv * env, jclass clazz)
  {
    return MY_TARGET_RB_KEY;
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
  Java_com_mapswithme_maps_PrivateVariables_googleWebClientId(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(GOOGLE_WEB_CLIENT_ID);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_adsRemovalServerId(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(ADS_REMOVAL_SERVER_ID);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_adsRemovalVendor(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(ADS_REMOVAL_VENDOR);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_adsRemovalYearlyProductId(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(ADS_REMOVAL_YEARLY_PRODUCT_ID);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_adsRemovalMonthlyProductId(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(ADS_REMOVAL_MONTHLY_PRODUCT_ID);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_adsRemovalWeeklyProductId(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(ADS_REMOVAL_WEEKLY_PRODUCT_ID);
  }

  JNIEXPORT jobjectArray JNICALL
  Java_com_mapswithme_maps_PrivateVariables_adsRemovalNotUsedList(JNIEnv * env, jclass clazz)
  {
    std::vector<std::string> items = ADS_REMOVAL_NOT_USED_LIST;
    return jni::ToJavaStringArray(env, items);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_bookmarksVendor(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(BOOKMARKS_VENDOR);
  }
}
