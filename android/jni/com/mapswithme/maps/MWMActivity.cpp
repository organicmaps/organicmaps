#include "Framework.hpp"

#include "../core/jni_helper.hpp"

#include "../platform/Platform.hpp"

#include "../../../nv_event/nv_event.hpp"

#include "../../../../../map/country_status_display.hpp"

#include "../../../../../platform/settings.hpp"


////////////////////////////////////////////////////////////////////////////////////////////
extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeLocationStatusChanged(JNIEnv * env, jobject thiz,
      int status)
  {
    g_framework->OnLocationStatusChanged(status);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeLocationUpdated(JNIEnv * env, jobject thiz,
      jlong time, jdouble lat, jdouble lon, jfloat accuracy)
  {
    g_framework->OnLocationUpdated(time, lat, lon, accuracy);
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

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MWMActivity_hasMeasurementSystem(JNIEnv * env, jobject thiz)
  {
    Settings::Units u;
    return Settings::Get("Units", u);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_setMeasurementSystem(JNIEnv * env, jobject thiz, jint systemIdx)
  {
    Settings::Units u = (Settings::Units)systemIdx;
    Settings::Set("Units", u);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_setupMeasurementSystem(JNIEnv * env, jobject thiz)
  {
    g_framework->SetupMeasurementSystem();
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_MWMActivity_getMeasurementSystem(JNIEnv * env, jobject thiz)
  {
    Settings::Units u = Settings::Metric;
    Settings::Get("Units", u);
    return u;
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
  Java_com_mapswithme_maps_MWMActivity_nativeSetString(JNIEnv * env, jobject thiz, jstring name, jstring value)
  {
    g_framework->AddString(jni::ToNativeString(name), jni::ToNativeString(value));
  }

#define SETTINGS_PRO_VERSION_URL_KEY "ProVersionURL"
#define SETTINGS_PRO_VERSION_LAST_CHECK_TIME "ProVersionLastCheck"

#define PRO_VERSION_CHECK_INTERVAL 2 * 60 * 60
//#define PRO_VERSION_CHECK_INTERVAL 10

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeIsProVersion(JNIEnv * env, jobject thiz)
  {
    return GetPlatform().IsFeatureSupported("search");
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeGetProVersionURL(JNIEnv * env, jobject thiz)
  {
    string res;
    Settings::Get(SETTINGS_PRO_VERSION_URL_KEY, res);
    return jni::ToJavaString(res);
  }

  void OnProVersionServerReply(downloader::HttpRequest & r, shared_ptr<jobject> obj)
  {
    if (r.Status() == downloader::HttpRequest::ECompleted)
    {
      string url = r.Data();

      LOG(LDEBUG, ("got response: ", url));

      if ((url.find("market://") == 0) || (url.find("http://") == 0))
      {
        LOG(LDEBUG, ("got PRO Version URL: ", url));
        Settings::Set(SETTINGS_PRO_VERSION_URL_KEY, url);

        JNIEnv * env = jni::GetEnv();

        jmethodID methodID = jni::GetJavaMethodID(jni::GetEnv(), *obj.get(), "onProVersionAvailable", "()V");

        env->CallVoidMethod(*obj.get(), methodID);
      }
      else
      {
        uint64_t curTime = time(0);
        Settings::Set(SETTINGS_PRO_VERSION_LAST_CHECK_TIME, strings::to_string(curTime));
        LOG(LDEBUG, ("ProVersion is not available, checkTime=", curTime));
      }
    }
    else
      LOG(LDEBUG, ("response finished with error"));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeCheckForProVersion(JNIEnv * env, jobject thiz, jstring proVersionServerURL)
  {
    string strLastCheckTime;

    bool shouldCheck = false;

    LOG(LDEBUG, ("figuring out whether we should check for pro version"));

    if (Settings::Get(SETTINGS_PRO_VERSION_LAST_CHECK_TIME, strLastCheckTime))
    {
      uint64_t lastCheckTime;
      if (!strings::to_uint64(strLastCheckTime, lastCheckTime))
        shouldCheck = true; //< value is corrupted or invalid, should re-check
      else
      {
        uint64_t curTime = time(0);
        if (curTime - lastCheckTime > PRO_VERSION_CHECK_INTERVAL)
          shouldCheck = true; //< last check was too long ago
        else
          LOG(LDEBUG, ("PRO_VERSION_CHECK_INTERVAL hasn't elapsed yet."));
      }
    }
    else
      shouldCheck = true; //< we haven't checked for PRO version yet

    if (shouldCheck)
    {
      LOG(LDEBUG, ("checking for Pro version"));
      downloader::HttpRequest::Get(jni::ToNativeString(proVersionServerURL), bind(&OnProVersionServerReply, _1, jni::make_global_ref(thiz)));
    }
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeProVersionUrl(JNIEnv * env, jobject thiz)
  {
    string res;
    Settings::Get(SETTINGS_PRO_VERSION_URL_KEY, res);
    return jni::ToJavaString(res);
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

} // extern "C"
