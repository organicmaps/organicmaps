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
    g_framework->AddString(jni::ToString(name), jni::ToString(value));
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
