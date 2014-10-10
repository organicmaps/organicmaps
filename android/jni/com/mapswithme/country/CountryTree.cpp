#include <jni.h>

#include "../maps/Framework.hpp"
#include "../core/jni_helper.hpp"
#include "country_helper.hpp"

using namespace storage_utils;

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_CountryTree_setDefaultRoot(JNIEnv * env, jclass clazz)
  {
    GetTree().SetDefaultRoot();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_CountryTree_setParentAsRoot(JNIEnv * env, jclass clazz)
  {
    GetTree().SetParentAsRoot();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_CountryTree_setChildAsRoot(JNIEnv * env, jclass clazz, jint position)
  {
    GetTree().SetChildAsRoot(position);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_CountryTree_resetRoot(JNIEnv * env, jclass clazz)
  {
    GetTree().ResetRoot();
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_country_CountryTree_hasParent(JNIEnv * env, jclass clazz)
  {
    return GetTree().HasParent();
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_country_CountryTree_getChildCount(JNIEnv * env, jclass clazz)
  {
    return GetTree().GetChildCount();
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_country_CountryTree_isLeaf(JNIEnv * env, jclass clazz, jint position)
  {
    return GetTree().IsLeaf(position);
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_country_CountryTree_getChildName(JNIEnv * env, jclass clazz, jint position)
  {
    return jni::ToJavaString(env, GetTree().GetChildName(position));
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_country_CountryTree_getLeafStatus(JNIEnv * env, jclass clazz, jint position)
  {
    return static_cast<jint>(GetTree().GetLeafStatus(position));
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_country_CountryTree_getLeafOptions(JNIEnv * env, jclass clazz, jint position)
  {
    return static_cast<jint>(GetTree().GetLeafOptions(position));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_CountryTree_downloadCountry(JNIEnv * env, jclass clazz, jint position, jint options)
  {
    GetTree().DownloadCountry(position, ToOptions(options));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_CountryTree_deleteCountry(JNIEnv * env, jclass clazz, jint position, jint options)
  {
    GetTree().DeleteCountry(position, ToOptions(options));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_CountryTree_cancelDownloading(JNIEnv * env, jclass clazz, jint position)
  {
    GetTree().CancelDownloading(position);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_CountryTree_retryDownloading(JNIEnv * env, jclass clazz, jint position)
  {
    // TODO uncomment when retry will be implemented
//    GetTree().RetryDownloading(position);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_CountryTree_showLeafOnMap(JNIEnv * env, jclass clazz, jint position)
  {
    GetTree().ShowLeafOnMap(position);
    g_framework->DontLoadState();
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_country_CountryTree_getLeafGuideInfo(JNIEnv * env, jclass clazz, jint position)
  {
    guides::GuideInfo info;
    if (GetTree().GetLeafGuideInfo(position, info))
      return guides::GuideNativeToJava(env, info);

    return NULL;
  }

  JNIEXPORT jlongArray JNICALL
  Java_com_mapswithme_country_CountryTree_getDownloadableLeafSize(JNIEnv * env, jclass clazz, jint position)
  {
    return ToArray(env, GetTree().GetDownloadableLeafSize(position));
  }

  JNIEXPORT jlongArray JNICALL
  Java_com_mapswithme_country_CountryTree_getLeafSize(JNIEnv * env, jclass clazz, jint position, jint options)
  {
    return ToArray(env, GetTree().GetLeafSize(position, ToOptions(options)));
  }

  JNIEXPORT jlongArray JNICALL
  Java_com_mapswithme_country_CountryTree_getRemoteLeafSizes(JNIEnv * env, jclass clazz, jint position)
  {
    return ToArray(env, GetTree().GetRemoteLeafSizes(position));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_CountryTree_setListener(JNIEnv * env, jclass clazz, jobject listener)
  {
    GetTree().SetListener(g_framework->setCountryTreeListener(jni::make_global_ref(listener)));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_CountryTree_resetListener(JNIEnv * env, jclass clazz, jobject listener)
  {
    g_framework->resetCountryTreeListener();
    GetTree().ResetListener();
  }
}
