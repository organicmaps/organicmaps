/*
 * DownloadUI.cpp
 *
 *  Created on: Oct 13, 2011
 *      Author: siarheirachytski
 */

#include <jni.h>
#include "Framework.hpp"

///////////////////////////////////////////////////////////////////////////////////
// DownloadUI
///////////////////////////////////////////////////////////////////////////////////

extern "C"
{
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
  Java_com_mapswithme_maps_DownloadUI_countrySizeInBytes(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    storage::LocalAndRemoteSizeT const s = g_framework->Storage().CountrySizeInBytes(storage::TIndex(group, country, region));
    // lower int contains remote size, and upper - local one
    int64_t mergedSize = s.second;
    mergedSize |= (s.first << 32);
    return mergedSize;
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_DownloadUI_countryStatus(JNIEnv * env, jobject thiz,
      jint group, jint country, jint region)
  {
    return static_cast<jint>(g_framework->Storage().CountryStatus(storage::TIndex(group, country, region)));
  }
}

