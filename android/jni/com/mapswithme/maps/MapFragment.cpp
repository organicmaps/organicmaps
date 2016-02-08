#include "Framework.hpp"

#include "../core/jni_helper.hpp"

#include "../platform/Platform.hpp"

#include "storage/index.hpp"

#include "base/logging.hpp"

#include "platform/file_logging.hpp"
#include "platform/settings.hpp"


extern "C"
{
using namespace storage;

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

static jobject g_this = nullptr;

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MapFragment_nativeConnectDownloaderListeners(JNIEnv * env, jobject thiz)
{
  g_this = env->NewGlobalRef(thiz);
  g_framework->NativeFramework()->SetDownloadCountryListener([](TCountryId const & countryId)
  {
    JNIEnv * env = jni::GetEnv();
    jmethodID methodID = jni::GetMethodID(env, g_this, "onDownloadClicked", "(Ljava/lang/String;)V");
    env->CallVoidMethod(g_this, methodID, jni::ToJavaString(env, countryId));
  });

  g_framework->NativeFramework()->SetDownloadCancelListener([](TCountryId const & countryId)
  {
    g_framework->Storage().CancelDownloadNode(countryId);
  });

  g_framework->NativeFramework()->SetAutoDownloadListener([](TCountryId const & countryId)
  {
    if (!g_framework->NeedMigrate() &&
        g_framework->IsAutodownloadMaps() &&
        Platform::ConnectionStatus() == Platform::EConnectionType::CONNECTION_WIFI)
    {
      g_framework->Storage().DownloadNode(countryId);
    }
  });
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MapFragment_nativeDisconnectListeners(JNIEnv * env, jclass clazz)
{
  g_framework->NativeFramework()->SetDownloadCountryListener(nullptr);
  g_framework->NativeFramework()->SetDownloadCancelListener(nullptr);
  g_framework->NativeFramework()->SetAutoDownloadListener(nullptr);

  if (g_this)
  {
    env->DeleteGlobalRef(g_this);
    g_this = nullptr;
  }
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
