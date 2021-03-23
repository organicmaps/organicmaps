#include "private.h"

#include "android/jni/com/mapswithme/core/jni_helper.hpp"

#include "coding/url.hpp"

#include <jni.h>

#include <string>

extern "C"
{
  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_bookmarksVendor(JNIEnv * env, jclass clazz)
  {
    return env->NewStringUTF(BOOKMARKS_VENDOR);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_bookmarksSubscriptionServerId(JNIEnv * env, jclass)
  {
    return env->NewStringUTF(BOOKMARKS_SUBSCRIPTION_SERVER_ID);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_bookmarksSubscriptionVendor(JNIEnv * env, jclass)
  {
    return env->NewStringUTF(BOOKMARKS_SUBSCRIPTION_VENDOR);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_bookmarksSubscriptionYearlyProductId(JNIEnv * env,
    jclass)
  {
    return env->NewStringUTF(BOOKMARKS_SUBSCRIPTION_YEARLY_PRODUCT_ID);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_bookmarksSubscriptionMonthlyProductId(JNIEnv * env,
    jclass)
  {
    return env->NewStringUTF(BOOKMARKS_SUBSCRIPTION_MONTHLY_PRODUCT_ID);
  }

  JNIEXPORT jobjectArray JNICALL
  Java_com_mapswithme_maps_PrivateVariables_bookmarksSubscriptionNotUsedList(JNIEnv * env, jclass)
  {
    std::vector<std::string> items = BOOKMARKS_SUBSCRIPTION_NOT_USED_LIST;
    return jni::ToJavaStringArray(env, items);
  }

  JNIEXPORT jobjectArray JNICALL
  Java_com_mapswithme_maps_PrivateVariables_bookmarkInAppIds(JNIEnv * env, jclass clazz)
  {
    std::vector<std::string> items = BOOKMARK_INAPP_IDS;
    return jni::ToJavaStringArray(env, items);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_bookmarksSubscriptionSightsServerId(JNIEnv * env, jclass)
  {
    return env->NewStringUTF(BOOKMARKS_SUBSCRIPTION_SIGHTS_SERVER_ID);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_bookmarksSubscriptionSightsYearlyProductId(JNIEnv * env,
                                                                                 jclass)
  {
    return env->NewStringUTF(BOOKMARKS_SUBSCRIPTION_SIGHTS_YEARLY_PRODUCT_ID);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_PrivateVariables_bookmarksSubscriptionSightsMonthlyProductId(JNIEnv * env,
                                                                                  jclass)
  {
    return env->NewStringUTF(BOOKMARKS_SUBSCRIPTION_SIGHTS_MONTHLY_PRODUCT_ID);
  }

  JNIEXPORT jobjectArray JNICALL
  Java_com_mapswithme_maps_PrivateVariables_bookmarksSubscriptionSightsNotUsedList(JNIEnv * env, jclass)
  {
    std::vector<std::string> items = BOOKMARKS_SUBSCRIPTION_SIGHTS_NOT_USED_LIST;
    return jni::ToJavaStringArray(env, items);
  }
}
