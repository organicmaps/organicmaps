#include "VideoTimer.hpp"

#include "../core/jni_helper.hpp"

#include "../../../../../base/assert.hpp"

static jobject g_smartGLSurfaceView = 0;
static jmethodID g_requestRenderMethodID;

extern "C"
{
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_SmartGLSurfaceView_nativeBind(JNIEnv * env, jobject thiz, jboolean isBound)
{
  if (isBound)
  {
    g_requestRenderMethodID = jni::GetJavaMethodID(env, thiz, "requestRender", "()V");
    ASSERT(g_requestRenderMethodID, ("Can't find method void com/mapswithme/maps/SmartGLSurfaceView.requestRender()"));

    g_smartGLSurfaceView = env->NewGlobalRef(thiz);
  }
  else
  {
    jobject refToDelete = g_smartGLSurfaceView;
    g_smartGLSurfaceView = 0;
    env->DeleteGlobalRef(refToDelete);
  }
}
}

namespace android
{
  VideoTimer::VideoTimer() : ::VideoTimer(TFrameFn())
  {
  }

  void VideoTimer::start()
  {
    if (g_smartGLSurfaceView)
    {
      JNIEnv * env = jni::GetEnv();
      ASSERT(env, ("JNIEnv is null"));
      env->CallVoidMethod(g_smartGLSurfaceView, g_requestRenderMethodID);
    }
  }

  void VideoTimer::resume()
  {
  }

  void VideoTimer::pause()
  {
  }

  void VideoTimer::stop()
  {
  }
}
