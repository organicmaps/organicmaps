#include "Framework.hpp"
#include "DownloadUI.hpp"

#include "../core/jni_helper.hpp"

#include "../../../../../std/bind.hpp"

android::DownloadUI * g_downloadUI = 0;

namespace android
{
  DownloadUI::DownloadUI(jobject self)
  {
    JNIEnv * env = jni::GetEnv();
    m_self = env->NewGlobalRef(self);

    jclass k = env->GetObjectClass(m_self);
    ASSERT(k, ("Can't get java class"));
    m_onChangeCountry = env->GetMethodID(k, "onChangeCountry", "(III)V");
    ASSERT(m_onChangeCountry, ("Can't get onChangeCountry methodID"));
    m_onProgress = env->GetMethodID(k, "onProgress", "(IIIJJ)V");
    ASSERT(m_onProgress, ("Can't get onProgress methodID"));

    ASSERT(!g_downloadUI, ("DownloadUI is initialized twice"));
    g_downloadUI = this;
  }

  DownloadUI::~DownloadUI()
  {
    g_downloadUI = 0;
    jni::GetEnv()->DeleteGlobalRef(m_self);
  }

  void DownloadUI::OnChangeCountry(storage::TIndex const & idx)
  {
    jni::GetEnv()->CallVoidMethod(m_self, m_onChangeCountry, idx.m_group, idx.m_country, idx.m_region);
  }

  void DownloadUI::OnProgress(storage::TIndex const & idx, pair<int64_t, int64_t> const & p)
  {
    jni::GetEnv()->CallVoidMethod(m_self, m_onProgress, idx.m_group, idx.m_country, idx.m_region, p.first, p.second);
  }
}

///////////////////////////////////////////////////////////////////////////////////
// DownloadUI
///////////////////////////////////////////////////////////////////////////////////

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_DownloadUI_nativeCreate(JNIEnv * env, jobject thiz)
  {
    if (g_downloadUI)
    {
      /// activity has been killed without onDestroy, destroying manually
      android::GetFramework().Storage().Unsubscribe();
      delete g_downloadUI;
      g_downloadUI = 0;
    }

    g_downloadUI = new android::DownloadUI(thiz);
    android::GetFramework().Storage().Subscribe(bind(&android::DownloadUI::OnChangeCountry, g_downloadUI, _1),
                                     bind(&android::DownloadUI::OnProgress, g_downloadUI, _1, _2));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_DownloadUI_nativeDestroy(JNIEnv * env, jobject thiz)
  {
    android::GetFramework().Storage().Unsubscribe();
    delete g_downloadUI;
    g_downloadUI = 0;
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_DownloadUI_countriesCount(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    return static_cast<jint>(android::GetFramework().Storage().CountriesCount(storage::TIndex(group, country, region)));
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_DownloadUI_countryName(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    string const name = android::GetFramework().Storage().CountryName(storage::TIndex(group, country, region));
    return env->NewStringUTF(name.c_str());
  }

  JNIEXPORT jlong JNICALL
  Java_com_mapswithme_maps_DownloadUI_countryLocalSizeInBytes(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    return android::GetFramework().Storage().CountrySizeInBytes(storage::TIndex(group, country, region)).first;
  }

  JNIEXPORT jlong JNICALL
  Java_com_mapswithme_maps_DownloadUI_countryRemoteSizeInBytes(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    return android::GetFramework().Storage().CountrySizeInBytes(storage::TIndex(group, country, region)).second;
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_DownloadUI_countryStatus(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    return static_cast<jint>(android::GetFramework().Storage().CountryStatus(storage::TIndex(group, country, region)));
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_DownloadUI_downloadCountry(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    android::GetFramework().Storage().DownloadCountry(storage::TIndex(group, country, region));
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_DownloadUI_deleteCountry(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    android::GetFramework().Storage().DeleteCountry(storage::TIndex(group, country, region));
  }
}

