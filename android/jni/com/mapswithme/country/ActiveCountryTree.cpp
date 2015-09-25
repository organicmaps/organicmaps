#include <jni.h>

#include "../maps/Framework.hpp"
#include "../maps/MapStorage.hpp"
#include "../core/jni_helper.hpp"
#include "country_helper.hpp"

using namespace storage_utils;
using namespace storage;

extern "C"
{
  JNIEXPORT jint JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_getOutOfDateCount(JNIEnv * env, jclass clazz)
  {
    return GetMapLayout().GetOutOfDateCount();
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_getCountInGroup(JNIEnv * env, jclass clazz, jint group)
  {
    return GetMapLayout().GetCountInGroup(ToGroup(group));
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_getCountryItem(JNIEnv * env, jclass clazz, jint group, jint position)
  {
    ActiveMapsLayout & layout = GetMapLayout();
    ActiveMapsLayout::TGroup coreGroup = ToGroup(group);
    int corePosition = static_cast<int>(position);
    jstring name = jni::ToJavaString(env, layout.GetCountryName(coreGroup, corePosition));
    jint status = static_cast<jint>(layout.GetCountryStatus(coreGroup, corePosition));
    jint options = static_cast<jint>(layout.GetCountryOptions(coreGroup, corePosition));

    jclass createClass = env->FindClass("com/mapswithme/country/CountryItem");
    ASSERT(createClass, ());

    jmethodID createMethodId = env->GetMethodID(createClass, "<init>", "(Ljava/lang/String;IIZ)V");
    ASSERT(createMethodId, ());

    return env->NewObject(createClass, createMethodId,
                          name, status, options, JNI_FALSE);
  }

  JNIEXPORT jlong JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_getCountrySize(JNIEnv * env, jclass clazz, jint group, jint position, jint options, jboolean isLocal)
  {
    ActiveMapsLayout & layout = GetMapLayout();
    ActiveMapsLayout::TGroup coreGroup = ToGroup(group);
    int pos = static_cast<int>(position);
    bool const local = isLocal == JNI_TRUE;
    MapOptions opt = ToOptions(options);

    if (options == -1 || local)
    {
      LocalAndRemoteSizeT sizes = options == -1 ? layout.GetDownloadableCountrySize(coreGroup, pos)
                                                : layout.GetCountrySize(coreGroup, pos, opt);
      return local ? sizes.first : sizes.second;
    }

    LocalAndRemoteSizeT sizes = layout.GetRemoteCountrySizes(coreGroup, pos);
    switch (opt)
    {
      case MapOptions::Map:
        return sizes.first;
      case MapOptions::CarRouting:
        return sizes.second;
      default:
        return sizes.first + sizes.second;
    }
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
    g_framework->PostDrapeTask([group, position]()
    {
      GetMapLayout().ShowMap(ToGroup(group), position);
    });
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
    return g_framework->AddActiveMapsListener(jni::make_global_ref(listener));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_removeListener(JNIEnv * env, jclass clazz, jint slotID)
  {
    g_framework->RemoveActiveMapsListener(slotID);
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_getCoreIndex(JNIEnv * env, jclass clazz, jint group, jint position)
  {
    return storage::ToJava(GetMapLayout().GetCoreIndex(static_cast<storage::ActiveMapsLayout::TGroup>(group),
                                                       static_cast<int>(position)));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_country_ActiveCountryTree_downloadMapForIndex(JNIEnv * env, jclass clazz, jobject index, jint options)
  {
    GetMapLayout().DownloadMap(storage::ToNative(index), ToOptions(options));
  }
}
