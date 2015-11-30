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
  Java_com_mapswithme_maps_MapFragment_nativeCompassUpdated(JNIEnv * env, jclass clazz, jdouble magneticNorth, jdouble trueNorth, jboolean forceRedraw)
  {
    location::CompassInfo info;
    info.m_bearing = (trueNorth >= 0.0) ? trueNorth : magneticNorth;

    g_framework->OnCompassUpdated(info, forceRedraw);
  }

#pragma clang pop_options

  JNIEXPORT jfloatArray JNICALL
  Java_com_mapswithme_maps_location_LocationHelper_nativeUpdateCompassSensor(JNIEnv * env, jclass clazz, jint ind, jfloatArray arr)
  {
    int const kCoordsCount = 3;

    // Extract coords
    jfloat coords[kCoordsCount];
    env->GetFloatArrayRegion(arr, 0, kCoordsCount, coords);
    g_framework->UpdateCompassSensor(ind, coords);

    // Put coords back to java result array
    jfloatArray ret = (jfloatArray)env->NewFloatArray(kCoordsCount);
    env->SetFloatArrayRegion(ret, 0, kCoordsCount, coords);
    return ret;
  }

  static void CallOnDownloadCountryClicked(shared_ptr<jobject> const & obj, storage::TIndex const & idx, int options, jmethodID methodID)
  {
    JNIEnv * env = jni::GetEnv();
    env->CallVoidMethod(*obj.get(), methodID, idx.m_group, idx.m_country, idx.m_region, options);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeConnectDownloadButton(JNIEnv * env, jobject thiz)
  {
    jmethodID methodID = jni::GetJavaMethodID(env, thiz, "onDownloadCountryClicked", "(IIII)V");
    g_framework->NativeFramework()->SetDownloadCountryListener(bind(&CallOnDownloadCountryClicked,
                                                               jni::make_global_ref(thiz), _1, _2, methodID));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeDownloadCountry(JNIEnv * env, jclass clazz, jobject idx, jint options)
  {
    storage::TIndex index = storage::ToNative(idx);
    storage::ActiveMapsLayout & layout = storage_utils::GetMapLayout();
    if (options == -1)
      layout.RetryDownloading(index);
    else
      layout.DownloadMap(index, storage_utils::ToOptions(options));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeStorageConnected(JNIEnv * env, jclass clazz)
  {
    android::Platform::Instance().OnExternalStorageStatusChanged(true);
    g_framework->AddLocalMaps();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeStorageDisconnected(JNIEnv * env, jclass clazz)
  {
    android::Platform::Instance().OnExternalStorageStatusChanged(false);
    g_framework->RemoveLocalMaps();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeScalePlus(JNIEnv * env, jclass clazz)
  {
    g_framework->Scale(::Framework::SCALE_MAG);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeScaleMinus(JNIEnv * env, jclass clazz)
  {
    g_framework->Scale(::Framework::SCALE_MIN);
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeShowMapForUrl(JNIEnv * env, jclass clazz, jstring url)
  {
    return g_framework->ShowMapForURL(jni::ToNativeString(env, url));
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeCreateEngine(JNIEnv * env, jclass clazz, jobject surface, jint density)
  {
    return static_cast<jboolean>(g_framework->CreateDrapeEngine(env, surface, static_cast<int>(density)));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeDestroyEngine(JNIEnv * env, jclass clazz)
  {
    g_framework->DeleteDrapeEngine();
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeIsEngineCreated(JNIEnv * env, jclass clazz)
  {
    return static_cast<jboolean>(g_framework->IsDrapeEngineCreated());
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeAttachSurface(JNIEnv * env, jclass clazz, jobject surface)
  {
    g_framework->AttachSurface(env, surface);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeDetachSurface(JNIEnv * env, jclass clazz)
  {
    g_framework->DetachSurface();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeSurfaceChanged(JNIEnv * env, jclass clazz, jint w, jint h)
  {
    g_framework->Resize(static_cast<int>(w), static_cast<int>(h));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeOnTouch(JNIEnv * env, jclass clazz, jint action,
                                                     jint id1, jfloat x1, jfloat y1,
                                                     jint id2, jfloat x2, jfloat y2,
                                                     jint maskedPointer)
  {
    g_framework->Touch(static_cast<int>(action),
                       android::Framework::Finger(id1, x1, y1),
                       android::Framework::Finger(id2, x2, y2), maskedPointer);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeSetupWidget(JNIEnv * env, jclass clazz, jint widget, jfloat x, jfloat y, jint anchor)
  {
    g_framework->SetupWidget(static_cast<gui::EWidget>(widget), static_cast<float>(x), static_cast<float>(y), static_cast<dp::Anchor>(anchor));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeApplyWidgets(JNIEnv * env, jclass clazz)
  {
    g_framework->ApplyWidgets();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapFragment_nativeCleanWidgets(JNIEnv * env, jclass clazz)
  {
    g_framework->CleanWidgets();
  }
} // extern "C"
