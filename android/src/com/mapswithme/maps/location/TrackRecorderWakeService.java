package com.mapswithme.maps.location;

import android.app.IntentService;
import android.content.Intent;
import android.support.annotation.Nullable;
import android.support.v4.content.WakefulBroadcastReceiver;

import java.lang.ref.WeakReference;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import com.mapswithme.maps.MwmApplication;

public class TrackRecorderWakeService extends IntentService
{
  private static WeakReference<TrackRecorderWakeService> sServiceRef;
  private final CountDownLatch mWaitMonitor = new CountDownLatch(1);

  private static @Nullable TrackRecorderWakeService getService()
  {
    if (sServiceRef == null)
      return null;

    TrackRecorderWakeService res = sServiceRef.get();
    if (res == null)
      sServiceRef = null;

    return res;
  }

  public TrackRecorderWakeService()
  {
    super("TrackRecorderWakeService");
  }

  @Override
  protected final void onHandleIntent(Intent intent)
  {
    synchronized (TrackRecorderWakeService.class)
    {
      TrackRecorder.log("SVC.onHandleIntent()");

      TrackRecorderWakeService svc = getService();
      if (svc != null)
      {
        TrackRecorder.log("SVC.onHandleIntent() SKIPPED because getService() returned something");
        return;
      }
      sServiceRef = new WeakReference<>(this);
    }

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

    synchronized (TrackRecorderWakeService.class)
    {
      sServiceRef = null;
    }

    TrackRecorder.onServiceStopped();
    WakefulBroadcastReceiver.completeWakefulIntent(intent);
  }

  public synchronized static void start()
  {
    TrackRecorder.log("SVC.start()");

    if (getService() == null)
      WakefulBroadcastReceiver.startWakefulService(MwmApplication.get(), new Intent(MwmApplication.get(), TrackRecorderWakeService.class));
    else
      TrackRecorder.log("SVC.start() SKIPPED because getService() returned something");
  }

  public synchronized static void stop()
  {
    TrackRecorder.log("SVC.stop()");

    TrackRecorderWakeService svc = getService();
    if (svc != null)
      svc.mWaitMonitor.countDown();
    else
      TrackRecorder.log("SVC.stop() SKIPPED because getService() returned nothing");
  }
}
