#include <jni.h>

#include "../maps/Framework.hpp"
#include "../core/jni_helper.hpp"
#include "country_helper.hpp"

using namespace storage_utils;

extern "C"
{
  JNIEXPORT jint JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_getCountInGroup(JNIEnv * env, jclass clazz, jint group)
  {
    return GetMapLayout().GetCountInGroup(ToGroup(group));
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_getCountryStatus(JNIEnv * env, jclass clazz, jint group, jint position)
  {
    return static_cast<jint>(GetMapLayout().GetCountryStatus(ToGroup(group), position));
  }

  JNIEXPORT jstring JNICALL Java_com_mapswithme_country_ActiveCountryTree_getCountryName(JNIEnv * env, jclass clazz, jint group, jint position)
  {
    return jni::ToJavaString(env, GetMapLayout().GetCountryName(ToGroup(group), position));
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_getCountryOptions(JNIEnv * env, jclass clazz, jint group, jint position)
  {
    return static_cast<jint>(GetMapLayout().GetCountryOptions(ToGroup(group), position));
  }

  JNIEXPORT jlongArray JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_getCountrySize(JNIEnv * env, jclass clazz, jint group, jint position, jint options)
  {
    return ToArray(env, GetMapLayout().GetCountrySize(ToGroup(group), position, ToOptions(options)));
  }

  JNIEXPORT jlongArray JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_getRemoteCountrySizes(JNIEnv * env, jclass clazz, jint group, jint position)
  {
    return ToArray(env, GetMapLayout().GetRemoteCountrySizes(ToGroup(group), position));
  }

  JNIEXPORT jlongArray JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_getDownloadableCountrySize(JNIEnv * env, jclass clazz, jint group, jint position)
  {
    return ToArray(env, GetMapLayout().GetDownloadableCountrySize(ToGroup(group), position));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_cancelDownloading(JNIEnv * env, jclass clazz, jint group, jint position)
  {
    GetMapLayout().CancelDownloading(ToGroup(group), position);
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_isDownloadingActive(JNIEnv * env, jclass clazz)
  {
    return GetMapLayout().IsDownloadingActive();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_retryDownloading(JNIEnv * env, jclass clazz, jint group, jint position)
  {
    GetMapLayout().RetryDownloading(ToGroup(group), position);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_downloadMap(JNIEnv * env, jclass clazz, jint group, jint position, jint options)
  {
    GetMapLayout().DownloadMap(ToGroup(group), position, ToOptions(options));
  }
  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_deleteMap(JNIEnv * env, jclass clazz, jint group, jint position, jint options)
  {
    GetMapLayout().DeleteMap(ToGroup(group), position, ToOptions(options));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_showOnMap(JNIEnv * env, jclass clazz, jint group, jint position)
  {
    GetMapLayout().ShowMap(ToGroup(group), position);
    g_framework->DontLoadState();
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_getGuideInfo(JNIEnv * env, jclass clazz, jint group, jint position)
  {
    guides::GuideInfo info;
    GetMapLayout().GetGuideInfo(ToGroup(group), position, info);
    return guides::GuideNativeToJava(env, info);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_updateAll(JNIEnv * env, jclass clazz)
  {
    GetMapLayout().UpdateAll();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_cancelAll(JNIEnv * env, jclass clazz)
  {
    GetMapLayout().CancelAll();
  }

  JNIEXPORT int JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_addListener(JNIEnv * env, jclass clazz, jobject listener)
  {
    return GetMapLayout().AddListener(g_framework->setActiveMapsListener(jni::make_global_ref(listener)));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_removeListener(JNIEnv * env, jclass clazz, jint slotID)
  {
    g_framework->resetActiveMapsListener();
    GetMapLayout().RemoveListener(slotID);
  }
}