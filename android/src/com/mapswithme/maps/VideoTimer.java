package com.mapswithme.maps;

import java.util.Timer;
import java.util.TimerTask;

public class VideoTimer
{

  private static String TAG = "VideoTimer";

  Timer m_timer;

  private native void nativeInit();

  private native void nativeRun();


  public class VideoTimerTask extends TimerTask
  {

    @Override
    public void run()
    {
      nativeRun();
    }
  }

  VideoTimerTask m_timerTask;
  int m_interval;

  public VideoTimer()
  {
    m_interval = 1000 / 60;
    nativeInit();
  }

  void start()
  {
    m_timerTask = new VideoTimerTask();
    m_timer = new Timer("VideoTimer");
    m_timer.scheduleAtFixedRate(m_timerTask, 0, m_interval);
  }

  void resume()
  {
    m_timerTask = new VideoTimerTask();
    m_timer = new Timer("VideoTimer");
    m_timer.scheduleAtFixedRate(m_timerTask, 0, m_interval);
  }

  void pause()
  {
    m_timer.cancel();
  }

  void stop()
  {
    m_timer.cancel();
  }

}
