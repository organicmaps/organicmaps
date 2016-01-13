package com.mapswithme.maps.location;

import android.app.IntentService;
import android.content.Intent;
import android.support.v4.content.WakefulBroadcastReceiver;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import com.mapswithme.maps.MwmApplication;

public class TrackRecorderWakeService extends IntentService
{
  private static volatile TrackRecorderWakeService sService;
  private final CountDownLatch mWaitMonitor = new CountDownLatch(1);

  public TrackRecorderWakeService()
  {
    super("TrackRecorderWakeService");
  }

  @Override
  protected final void onHandleIntent(Intent intent)
  {
    TrackRecorder.log("SVC.onHandleIntent()");

    sService = this;
    TrackRecorder.onServiceStarted();

    try
    {
      long timeout = TrackRecorder.getAwaitTimeout();
      TrackRecorder.log("Timeout: " + timeout);

      if (!mWaitMonitor.await(timeout, TimeUnit.MILLISECONDS))
      {
        TrackRecorder.log("TIMEOUT awaiting coordinates");
        TrackRecorder.incrementAwaitTimeout();
      }
    } catch (InterruptedException ignored) {}

    sService = null;

    TrackRecorder.onServiceStopped();
    WakefulBroadcastReceiver.completeWakefulIntent(intent);
  }

  public static void start()
  {
    TrackRecorder.log("SVC.start()");

    if (sService == null)
      WakefulBroadcastReceiver.startWakefulService(MwmApplication.get(), new Intent(MwmApplication.get(), TrackRecorderWakeService.class));
    else
      TrackRecorder.log("SVC.start() SKIPPED because (sService != null)");
  }

  public static void stop()
  {
    TrackRecorder.log("SVC.stop()");

    if (sService != null)
      sService.mWaitMonitor.countDown();
    else
      TrackRecorder.log("SVC.stop() SKIPPED because (sService == null)");
  }
}
