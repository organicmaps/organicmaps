/*
 * VideoTimer.cpp
 *
 *  Created on: Nov 5, 2011
 *      Author: siarheirachytski
 */

#include "../core/jni_helper.hpp"
#include "VideoTimer.hpp"
#include "../../../../../base/assert.hpp"
#include "../../../../../base/logging.hpp"

android::VideoTimer * g_timer = 0;

jclass g_smartGlViewClass = 0;
jmethodID g_smartGlViewRedrawMethodId = 0;

extern "C"
{
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_SmartGLSurfaceView_nativeSetRedraw(JNIEnv * env, jobject thiz, jboolean isValid)
{
//  if (!g_smartGlViewClass)
//    g_smartGlViewClass = env->GetObjectClass(thiz);
//  if (isValid)
//    g_smartGlViewRedrawMethodId = env->GetMethodID(g_smartGlViewClass, "requestRender", "()V");
//  else
//    g_smartGlViewRedrawMethodId = 0;
}
}

namespace android
{
  VideoTimer::VideoTimer(JavaVM * javaVM, TFrameFn frameFn)
    : m_javaVM(javaVM), ::VideoTimer(frameFn)
  {
    ASSERT(g_timer == 0, ());
    g_timer = this;
  }

  VideoTimer::~VideoTimer()
  {
    stop();
    g_timer = 0;
  }

  void VideoTimer::SetParentObject(jobject videoTimer)
  {
    m_videoTimer = videoTimer;
  }

  void VideoTimer::start()
  {
    /*JNIEnv * env;
    m_javaVM->AttachCurrentThread(&env, NULL);
    env->CallVoidMethod(m_videoTimer, jni::GetJavaMethodID(env, m_videoTimer, "start", "()V"));*/
    //m_state = ERunning;

//    if (!g_smartGlViewRedrawMethodId)
//    {
//      ASSERT(m_javaVM, ("m_jvm is NULL"));
//      JNIEnv * env = 0;
//      if (m_javaVM->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK)
//      {
//        if (m_javaVM->AttachCurrentThread(&env, 0) != JNI_OK)
//        {
//          LOG(LWARNING, ("Can't attach thread"));
//          return;
//        }
//      }
//
//      env->CallStaticVoidMethod(g_smartGlViewClass, g_smartGlViewRedrawMethodId);
//    }
  }

  void VideoTimer::resume()
  {
    /*JNIEnv * env;
    m_javaVM->AttachCurrentThread(&env, NULL);
    env->CallVoidMethod(m_videoTimer, jni::GetJavaMethodID(env, m_videoTimer, "resume", "()V"));*/
    //m_state = ERunning;
  }

  void VideoTimer::pause()
  {
    /*JNIEnv * env;
    m_javaVM->AttachCurrentThread(&env, NULL);
    env->CallVoidMethod(m_videoTimer, jni::GetJavaMethodID(env, m_videoTimer, "pause", "()V"));*/
    //m_state = EPaused;
  }

  void VideoTimer::stop()
  {
    /*JNIEnv * env;
    m_javaVM->AttachCurrentThread(&env, NULL);
    env->CallVoidMethod(m_videoTimer, jni::GetJavaMethodID(env, m_videoTimer, "stop", "()V"));*/
    //m_state = EStopped;
  }

  void VideoTimer::perform()
  {
    //m_frameFn();
  }
}

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_VideoTimer_nativeRun(JNIEnv * env, jobject thiz)
  {
    ASSERT ( g_timer, ());
    g_timer->perform();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_VideoTimer_nativeInit(JNIEnv * env, jobject thiz)
  {
    ASSERT ( g_timer, ());
    g_timer->SetParentObject(thiz);
  }
}



