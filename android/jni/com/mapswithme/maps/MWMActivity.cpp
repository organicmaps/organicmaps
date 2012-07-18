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

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeSetMS(JNIEnv * env, jobject thiz, jint systemIdx)
  {
    Settings::Units const u = static_cast<Settings::Units>(systemIdx);
    Settings::Set("Units", u);
    g_framework->SetupMeasurementSystem();
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeGetMS(JNIEnv * env, jobject thiz)
  {
    Settings::Units u = Settings::Metric;
    return (Settings::Get("Units", u) ? u : -1);
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
    g_framework->AddString(jni::ToNativeString(env, name),
                           jni::ToNativeString(env, value));
  }

#define SETTINGS_PRO_VERSION_URL_KEY "ProVersionURL"
#define SETTINGS_PRO_VERSION_LAST_CHECK_TIME "ProVersionLastCheck"

#define PRO_VERSION_CHECK_INTERVAL 2 * 60 * 60
//#define PRO_VERSION_CHECK_INTERVAL 10

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_MWMActivity_nativeGetProVersionURL(JNIEnv * env, jobject thiz)
  {
    string res;
    Settings::Get(SETTINGS_PRO_VERSION_URL_KEY, res);
    return jni::ToJavaString(env, res);
  }

  void OnProVersionServerReply(downloader::HttpRequest & r, shared_ptr<jobject> obj)
  {
    uint64_t const curTime = time(0);

    if (r.Status() == downloader::HttpRequest::ECompleted)
    {
      string url = r.Data();

      LOG(LDEBUG, ("got response: ", url));

      if ((url.find("https://") == 0)
       || (url.find("market://") == 0)
       || (url.find("http://") == 0))
      {
        LOG(LDEBUG, ("ProVersion URL is available: ", url));
        Settings::Set(SETTINGS_PRO_VERSION_URL_KEY, url);

        JNIEnv * env = jni::GetEnv();

        jmethodID methodID = jni::GetJavaMethodID(env, *obj.get(), "onProVersionAvailable", "()V");

        env->CallVoidMethod(*obj.get(), methodID);
      }
      else
        LOG(LDEBUG, ("ProVersion is not available, checkTime=", curTime));
    }
    else
      LOG(LDEBUG, ("ProVersion check response finished with error"));

    Settings::Set(SETTINGS_PRO_VERSION_LAST_CHECK_TIME, strings::to_string(curTime));
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
        uint64_t const curTime = time(0);
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
      downloader::HttpRequest::Get(jni::ToNativeString(env, proVersionServerURL),
                                   bind(&OnProVersionServerReply, _1, jni::make_global_ref(thiz)));
    }
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
} // extern "C"
