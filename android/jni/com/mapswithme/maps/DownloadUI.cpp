#include <jni.h>
#include "Framework.hpp"
#include "DownloadUI.hpp"
#include "../jni/jni_thread.hpp"
#include "../../../../../std/bind.hpp"

android::DownloadUI * g_downloadUI = 0;

namespace android
{
  DownloadUI::DownloadUI(jobject self)
  {
    m_self = jni::GetCurrentThreadJNIEnv()->NewGlobalRef(self);

    jclass k = jni::GetCurrentThreadJNIEnv()->GetObjectClass(m_self);

    m_onChangeCountry.reset(new jni::Method(k, "onChangeCountry", "(III)V"));
    m_onProgress.reset(new jni::Method(k, "onProgress", "(IIIJJ)V"));

    ASSERT(!g_downloadUI, ("DownloadUI is initialized twice"));
    g_downloadUI = this;
  }

  DownloadUI::~DownloadUI()
  {
    jni::GetCurrentThreadJNIEnv()->DeleteGlobalRef(m_self);
    g_downloadUI = 0;
  }

  void DownloadUI::OnChangeCountry(storage::TIndex const & idx)
  {
    m_onChangeCountry->CallVoid(m_self, idx.m_group, idx.m_country, idx.m_region);
  }

  void DownloadUI::OnProgress(storage::TIndex const & idx, pair<int64_t, int64_t> const & p)
  {
    m_onProgress->CallVoid(m_self, idx.m_group, idx.m_country, idx.m_region, p.first, p.second);
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
      g_framework->Storage().Unsubscribe();
      delete g_downloadUI;
      g_downloadUI = 0;
    }

    g_downloadUI = new android::DownloadUI(thiz);
    g_framework->Storage().Subscribe(bind(&android::DownloadUI::OnChangeCountry, g_downloadUI, _1),
                                     bind(&android::DownloadUI::OnProgress, g_downloadUI, _1, _2));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_DownloadUI_nativeDestroy(JNIEnv * env, jobject thiz)
  {
    g_framework->Storage().Unsubscribe();
    delete g_downloadUI;
    g_downloadUI = 0;
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_DownloadUI_countriesCount(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    return static_cast<jint>(g_framework->Storage().CountriesCount(storage::TIndex(group, country, region)));
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_DownloadUI_countryName(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    string const name = g_framework->Storage().CountryName(storage::TIndex(group, country, region));
    return env->NewStringUTF(name.c_str());
  }

  JNIEXPORT jlong JNICALL
  Java_com_mapswithme_maps_DownloadUI_countryLocalSizeInBytes(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    return g_framework->Storage().CountrySizeInBytes(storage::TIndex(group, country, region)).first;
  }

  JNIEXPORT jlong JNICALL
  Java_com_mapswithme_maps_DownloadUI_countryRemoteSizeInBytes(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    return g_framework->Storage().CountrySizeInBytes(storage::TIndex(group, country, region)).second;
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_DownloadUI_countryStatus(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    return static_cast<jint>(g_framework->Storage().CountryStatus(storage::TIndex(group, country, region)));
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_DownloadUI_downloadCountry(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    g_framework->Storage().DownloadCountry(storage::TIndex(group, country, region));
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_DownloadUI_deleteCountry(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    g_framework->Storage().DeleteCountry(storage::TIndex(group, country, region));
  }
}

