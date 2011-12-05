/*
 * DownloadUI.cpp
 *
 *  Created on: Oct 13, 2011
 *      Author: siarheirachytski
 */

#include <jni.h>
#include "Framework.hpp"
#include "DownloadUI.hpp"
#include "../jni/jni_thread.hpp"
#include "../../../../../std/bind.hpp"
#include "../../../../../base/logging.hpp"

android::DownloadUI * g_downloadUI = 0;

namespace android
{
  DownloadUI::DownloadUI(jobject self)
  {
    m_self = jni::GetCurrentThreadJNIEnv()->NewGlobalRef(self);

    jclass k = jni::GetCurrentThreadJNIEnv()->GetObjectClass(m_self);

    m_onChangeCountry.reset(new jni::Method(k, "onChangeCountry", "(III)V"));
    m_onProgress.reset(new jni::Method(k, "onProgress", "(IIIJJ)V"));

    ASSERT(!g_downloadUI, ());
    g_downloadUI = this;
  }

  DownloadUI::~DownloadUI()
  {
    jni::GetCurrentThreadJNIEnv()->DeleteGlobalRef(m_self);
    g_downloadUI = 0;
  }

  void DownloadUI::OnChangeCountry(storage::TIndex const & idx)
  {
    jint group = idx.m_group;
    jint country = idx.m_country;
    jint region = idx.m_region;

    LOG(LINFO, ("Changed Country", group, country, region));

    m_onChangeCountry->CallVoid(m_self, group, country, region);
  }

  void DownloadUI::OnProgress(storage::TIndex const & idx, pair<int64_t, int64_t> const & p)
  {
    jint group = idx.m_group;
    jint country = idx.m_country;
    jint region = idx.m_region;
    jlong p1 = p.first;
    jlong p2 = p.second;

    LOG(LINFO, ("Country Progress", group, country, region, p1, p2));

    m_onProgress->CallVoid(m_self, group, country, region, p1, p2);
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
    g_downloadUI = new android::DownloadUI(thiz);
    g_framework->Storage().Subscribe(bind(&android::DownloadUI::OnChangeCountry, g_downloadUI, _1),
                                     bind(&android::DownloadUI::OnProgress, g_downloadUI, _1, _2));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_DownloadUI_nativeDestroy(JNIEnv * env, jobject thiz)
  {
    g_framework->Storage().Unsubscribe();
    delete g_downloadUI;
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
    storage::LocalAndRemoteSizeT const s = g_framework->Storage().CountrySizeInBytes(storage::TIndex(group, country, region));
    return s.first;
  }

  JNIEXPORT jlong JNICALL
  Java_com_mapswithme_maps_DownloadUI_countryRemoteSizeInBytes(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    storage::LocalAndRemoteSizeT const s = g_framework->Storage().CountrySizeInBytes(storage::TIndex(group, country, region));
    return s.second;
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

