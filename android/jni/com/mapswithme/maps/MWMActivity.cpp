#include "Framework.hpp"

#include "../core/jni_helper.hpp"

#include "../platform/Platform.hpp"

#include "../../../nv_event/nv_event.hpp"
#include "../../../../../base/logging.hpp"

#include "../../../../../map/country_status_display.hpp"
////////////////////////////////////////////////////////////////////////////////////////////
extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_deactivatePopup(JNIEnv * env, jobject thiz)
  {
    return g_framework->DeactivatePopup();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeOnLocationError(JNIEnv * env, jobject thiz,
      int errorCode)
  {
    g_framework->OnLocationError(errorCode);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeLocationUpdated(JNIEnv * env, jobject thiz,
      jlong time, jdouble lat, jdouble lon, jfloat accuracy,
      jdouble altitude, jfloat speed, jfloat bearing)
  {
    const double GPS_VALUE_NOT_SET = -9999999.9;

    location::GpsInfo info;
    info.m_horizontalAccuracy = accuracy;
    info.m_latitude = lat;
    info.m_longitude = lon;
    info.m_timestamp = time;
    info.m_source = location::EAndroidNative;

    if (altitude == 0.0)
      info.m_altitude = info.m_verticalAccuracy = GPS_VALUE_NOT_SET;
    else
    {
      info.m_altitude = altitude;
      // use horizontal accuracy
      info.m_verticalAccuracy = accuracy;
    }
    info.m_course = (bearing == 0.0 ? GPS_VALUE_NOT_SET : bearing);
    info.m_speed = (speed == 0.0 ? GPS_VALUE_NOT_SET : speed);

    g_framework->OnLocationUpdated(info);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeCompassUpdated(JNIEnv * env, jobject thiz,
      jlong time, jdouble magneticNorth, jdouble trueNorth, jdouble accuracy)
  {
    g_framework->OnCompassUpdated(time, magneticNorth, trueNorth, accuracy);
  }

  JNIEXPORT jfloatArray JNICALL
  Java_com_mapswithme_maps_location_LocationService_nativeUpdateCompassSensor(
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
                                    jmethodID methodID)
  {
    JNIEnv * env = jni::GetEnv();
    env->CallVoidMethod(*obj.get(), methodID);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeConnectDownloadButton(JNIEnv * env, jobject thiz)
  {
    CountryStatusDisplay * display = g_framework->GetCountryStatusDisplay();

    jmethodID methodID = jni::GetJavaMethodID(env, thiz, "OnDownloadCountryClicked", "()V");

    display->setDownloadListener(bind(&CallOnDownloadCountryClicked,
                                       jni::make_global_ref(thiz),
                                       methodID));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeDownloadCountry(JNIEnv * env, jobject thiz)
  {
    g_framework->GetCountryStatusDisplay()->downloadCountry();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeStorageConnected(JNIEnv * env, jobject thiz)
  {
    android::Platform::Instance().OnExternalStorageStatusChanged(true);
    g_framework->AddLocalMaps();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeStorageDisconnected(JNIEnv * env, jobject thiz)
  {
    android::Platform::Instance().OnExternalStorageStatusChanged(false);
    g_framework->RemoveLocalMaps();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeScale(JNIEnv * env, jobject thiz, jdouble k)
  {
    g_framework->Scale(static_cast<double>(k));
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeIsInChina(JNIEnv * env, jobject thiz,
                                                       jdouble lat, jdouble lon)
  {
    char const * arr[] = { "cn", "hk", "mo" };

    string const code = g_framework->GetCountryCode(lat, lon);
    LOG(LDEBUG, ("Current country ISO code = ", code));

    for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
      if (code == arr[i])
        return true;

    return false;
  }


  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MWMActivity_setViewPortByUrl(JNIEnv * env, jobject thiz, jstring url)
  {
    return g_framework->SetViewportByUrl(jni::ToNativeString(env, url));
  }
} // extern "C"
