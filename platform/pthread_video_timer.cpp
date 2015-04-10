#include "std/target_os.hpp"

#ifndef OMIM_OS_WINDOWS_NATIVE

#include "platform/video_timer.hpp"

#include "base/logging.hpp"

#include <pthread.h>
#include <sys/time.h>
#include <sys/errno.h>


class PThreadVideoTimer : public VideoTimer
{
private:

  pthread_t m_handle;
  pthread_mutex_t m_mutex;
  pthread_cond_t m_cond;
  int m_frameRate;

public:
  PThreadVideoTimer(VideoTimer::TFrameFn frameFn)
    : VideoTimer(frameFn), m_frameRate(60)
  {
    ::pthread_mutex_init(&m_mutex, 0);
    ::pthread_cond_init(&m_cond, 0);
  }

  ~PThreadVideoTimer()
  {
    stop();

    ::pthread_mutex_destroy(&m_mutex);
    ::pthread_cond_destroy(&m_cond);
  }

  static void * TimerProc(void * p)
  {
    PThreadVideoTimer * t = reinterpret_cast<PThreadVideoTimer*>(p);

    ::timeval prevtv;
    ::gettimeofday(&prevtv, 0);

    ::timeval curtv;

    int64_t interval = 1000000000 / t->m_frameRate;
    int64_t halfInterval = interval / 2;

    while (true)
    {
      ::pthread_mutex_lock(&t->m_mutex);

      t->perform();

      ::gettimeofday(&curtv, 0);

      int64_t sec = (int64_t)curtv.tv_sec - (int64_t)prevtv.tv_sec;
      int64_t nsec = ((int64_t)curtv.tv_usec - (int64_t)prevtv.tv_usec) * 1000;

      int64_t nsecDiff = sec * (int64_t)1000000000 + nsec;
      int64_t ceiledDiff = ((nsecDiff + interval - 1) / interval) * interval;

      /// how much we should wait

      if ((ceiledDiff - nsecDiff) < halfInterval)
        /// less than a half-frame left, should wait till next frame
        ceiledDiff += interval;

      ::timespec ts;

      ts.tv_sec = prevtv.tv_sec + (prevtv.tv_usec * 1000 + ceiledDiff) / 1000000000;
      ts.tv_nsec = (prevtv.tv_usec * 1000 + ceiledDiff) % 1000000000;

      ::pthread_cond_timedwait(&t->m_cond, &t->m_mutex, &ts);

      ::gettimeofday(&prevtv, 0);

      if (t->m_state == EStopped)
      {
        ::pthread_mutex_unlock(&t->m_mutex);
        break;
      }

      ::pthread_mutex_unlock(&t->m_mutex);
    }

    ::pthread_exit(0);

    return 0;
  }

  void start()
  {
    if (m_state == EStopped)
    {
      ::pthread_create(&m_handle, 0, &TimerProc, reinterpret_cast<void*>(this));
      m_state = ERunning;
    }
  }

  void resume()
  {
    if (m_state == EPaused)
    {
      m_state = EStopped;
      start();
    }
  }

  void pause()
  {
    stop();
    m_state = EPaused;
  }

  void stop()
  {
    if (m_state == ERunning)
    {
      ::pthread_mutex_lock(&m_mutex);
      m_state = EStopped;
      ::pthread_cond_signal(&m_cond);
      ::pthread_mutex_unlock(&m_mutex);
      ::pthread_join(m_handle, 0);
    }
  }

  void perform()
  {
    m_frameFn();
  }
};

VideoTimer * CreatePThreadVideoTimer(VideoTimer::TFrameFn frameFn)
{
  return new PThreadVideoTimer(frameFn);
}

#endif
