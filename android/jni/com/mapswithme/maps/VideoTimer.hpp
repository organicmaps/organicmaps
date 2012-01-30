/*
 * VideoTimer.hpp
 *
 *  Created on: Nov 5, 2011
 *      Author: siarheirachytski
 */

#pragma once

#include <jni.h>
#include "../../../../../platform/video_timer.hpp"

namespace android
{
  class VideoTimer : public ::VideoTimer
  {
  private:
    JavaVM * m_javaVM;

    jobject m_videoTimer;

  public:

    VideoTimer(JavaVM * jvm, TFrameFn frameFn);
    ~VideoTimer();

    void SetParentObject(jobject videoTimer);

    void start();

    void stop();

    void resume();

    void pause();

    void perform();
  };
}

extern android::VideoTimer * g_timer;
