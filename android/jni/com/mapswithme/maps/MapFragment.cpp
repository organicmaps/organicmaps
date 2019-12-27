#include "Framework.hpp"

#include "com/mapswithme/core/jni_helper.hpp"

#include "com/mapswithme/platform/Platform.hpp"

#include "storage/storage_defines.hpp"

#include "base/logging.hpp"

#include "platform/file_logging.hpp"
#include "platform/settings.hpp"

namespace
{
void OnRenderingInitializationFinished(std::shared_ptr<jobject> const & listener)
{
  JNIEnv * env = jni::GetEnv();
  env->CallVoidMethod(*listener, jni::GetMethodID(env, *listener.get(),
                      "onRenderingInitializationFinished", "()V"));
}
}  // namespace

extern "C"
{
using namespace storage;

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MapFragment_nativeCompassUpdated(JNIEnv * env, jclass clazz, jdouble north, jboolean forceRedraw)
{
  location::CompassInfo info;
  info.m_bearing = north;

  g_framework->OnCompassUpdated(info, forceRedraw);
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
Java_com_mapswithme_maps_MapFragment_nativeCreateEngine(JNIEnv * env, jclass clazz,
                                                        jobject surface, jint density,
                                                        jboolean firstLaunch,
                                                        jboolean isLaunchByDeepLink,
                                                        jint appVersionCode)
{
  return g_framework->CreateDrapeEngine(env, surface, density, firstLaunch, isLaunchByDeepLink,
                                        appVersionCode);
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_MapFragment_nativeIsEngineCreated(JNIEnv * env, jclass clazz)
{
  return g_framework->IsDrapeEngineCreated();
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_MapFragment_nativeDestroySurfaceOnDetach(JNIEnv * env, jclass clazz)
{
  return g_framework->DestroySurfaceOnDetach();
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_MapFragment_nativeAttachSurface(JNIEnv * env, jclass clazz,
                                                         jobject surface)
{
  return g_framework->AttachSurface(env, surface);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MapFragment_nativeDetachSurface(JNIEnv * env, jclass clazz,
                                                         jboolean destroySurface)
{
  g_framework->DetachSurface(destroySurface);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MapFragment_nativePauseSurfaceRendering(JNIEnv *, jclass)
{
  g_framework->PauseSurfaceRendering();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MapFragment_nativeResumeSurfaceRendering(JNIEnv *, jclass)
{
  g_framework->ResumeSurfaceRendering();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MapFragment_nativeSurfaceChanged(JNIEnv * env, jclass, jobject surface,
                                                          jint w, jint h)
{
  g_framework->Resize(env, surface, w, h);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MapFragment_nativeOnTouch(JNIEnv * env, jclass clazz, jint action,
                                                   jint id1, jfloat x1, jfloat y1,
                                                   jint id2, jfloat x2, jfloat y2,
                                                   jint maskedPointer)
{
  g_framework->Touch(action,
                     android::Framework::Finger(id1, x1, y1),
                     android::Framework::Finger(id2, x2, y2), maskedPointer);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MapFragment_nativeSetupWidget(
  JNIEnv * env, jclass clazz, jint widget, jfloat x, jfloat y, jint anchor)
{
  g_framework->SetupWidget(static_cast<gui::EWidget>(widget), x, y, static_cast<dp::Anchor>(anchor));
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

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_MapFragment_nativeSetRenderingInitializationFinishedListener(
  JNIEnv * env, jclass clazz, jobject listener)
{
  if (listener)
  {
    g_framework->NativeFramework()->SetGraphicsContextInitializationHandler(
      std::bind(&OnRenderingInitializationFinished, jni::make_global_ref(listener)));
  }
  else
  {
    g_framework->NativeFramework()->SetGraphicsContextInitializationHandler(nullptr);
  }
}
} // extern "C"
