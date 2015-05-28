#include "Framework.hpp"
#include "MapStorage.hpp"

#include "../country/country_helper.hpp"

#include "../core/jni_helper.hpp"

#include "../platform/Platform.hpp"

#include "storage/index.hpp"

#include "base/logging.hpp"

#include "platform/file_logging.hpp"


extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeOnLocationError(JNIEnv * env, jobject thiz,
      int errorCode)
  {
    g_framework->OnLocationError(errorCode);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeLocationUpdated(JNIEnv * env, jobject thiz,
      jlong time, jdouble lat, jdouble lon,
      jfloat accuracy, jdouble altitude, jfloat speed, jfloat bearing)
  {
    location::GpsInfo info;
    info.m_source = location::EAndroidNative;

    info.m_timestamp = static_cast<double>(time) / 1000.0;
    info.m_latitude = lat;
    info.m_longitude = lon;

    if (accuracy > 0.0)
      info.m_horizontalAccuracy = accuracy;

    if (altitude != 0.0)
    {
      info.m_altitude = altitude;
      info.m_verticalAccuracy = accuracy;
    }

    if (bearing > 0.0)
      info.m_bearing = bearing;

    if (speed > 0.0)
      info.m_speed = speed;

    LOG_MEMORY_INFO();    
    g_framework->OnLocationUpdated(info);
  }

// Fixed optimization bug for x86 (reproduced on Asus ME302C).
#pragma clang push_options
#pragma clang optimize off

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeCompassUpdated(JNIEnv * env, jobject thiz,
      jdouble magneticNorth, jdouble trueNorth, jboolean force)
  {
    location::CompassInfo info;
    info.m_bearing = (trueNorth >= 0.0) ? trueNorth : magneticNorth;

    g_framework->OnCompassUpdated(info, force);
  }

#pragma clang pop_options

  JNIEXPORT jfloatArray JNICALL
  Java_com_mapswithme_maps_location_LocationHelper_nativeUpdateCompassSensor(
      JNIEnv * env, jobject thiz, jint ind, jfloatArray arr)
  {
    int const count = 3;

    // get Java array
    jfloat buffer[3];
    env->GetFloatArrayRegion(arr, 0, count, buffer);

    // get the result
    g_framework->UpdateCompassSensor(ind, buffer);

    // pass result back to Java
    jfloatArray ret = (jfloatArray)env->NewFloatArray(count);
    env->SetFloatArrayRegion(ret, 0, count, buffer);
    return ret;
  }

  void CallOnDownloadCountryClicked(shared_ptr<jobject> const & obj,
                                    storage::TIndex const & idx,
                                    int options,
                                    jmethodID methodID)
  {
    JNIEnv * env = jni::GetEnv();
    env->CallVoidMethod(*obj.get(), methodID, idx.m_group, idx.m_country, idx.m_region, options);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeConnectDownloadButton(JNIEnv * env, jobject thiz)
  {
    ///@TODO UVR
    //CountryStatusDisplay * display = g_framework->GetCountryStatusDisplay();

    //jmethodID methodID = jni::GetJavaMethodID(env, thiz, "OnDownloadCountryClicked", "(IIII)V");

    //display->SetDownloadCountryListener(bind(&CallOnDownloadCountryClicked,
    //                                          jni::make_global_ref(thiz),
    //                                          _1,
    //                                          _2,
    //                                          methodID));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeDownloadCountry(JNIEnv * env, jobject thiz, jobject idx, jint options)
  {
    storage::TIndex index = storage::ToNative(idx);
    storage::ActiveMapsLayout & layout = storage_utils::GetMapLayout();
    if (options == -1)
      layout.RetryDownloading(index);
    else
      layout.DownloadMap(index, storage_utils::ToOptions(options));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeStorageConnected(JNIEnv * env, jobject thiz)
  {
    android::Platform::Instance().OnExternalStorageStatusChanged(true);
    g_framework->AddLocalMaps();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeStorageDisconnected(JNIEnv * env, jobject thiz)
  {
    android::Platform::Instance().OnExternalStorageStatusChanged(false);
    g_framework->RemoveLocalMaps();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeScalePlus(JNIEnv * env, jobject thiz)
  {
    g_framework->Scale(::Framework::SCALE_MAG);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeScaleMinus(JNIEnv * env, jobject thiz)
  {
    g_framework->Scale(::Framework::SCALE_MIN);
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MapFragment_showMapForUrl(JNIEnv * env, jobject thiz, jstring url)
  {
    return g_framework->ShowMapForURL(jni::ToNativeString(env, url));
  }
} // extern "C"
