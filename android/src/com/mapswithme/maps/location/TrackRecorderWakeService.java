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
  private static final long TIMEOUT_MS = 30000;
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

  protected void cancel()
  {
    mWaitMonitor.countDown();
  }

  @Override
  protected final void onHandleIntent(Intent intent)
  {
    synchronized (TrackRecorderWakeService.class)
    {
      TrackRecorderWakeService svc = getService();
      if (svc != null)
        return;

      sServiceRef = new WeakReference<>(this);
    }

    TrackRecorder.onServiceStarted();

    try
    {
      mWaitMonitor.await(TIMEOUT_MS, TimeUnit.MILLISECONDS);
    } catch (InterruptedException ignored) {}

    synchronized (getClass())
    {
      sServiceRef = null;
    }

    TrackRecorder.onServiceStopped();
    WakefulBroadcastReceiver.completeWakefulIntent(intent);
  }

  public synchronized static void start()
  {
    if (getService() == null)
      WakefulBroadcastReceiver.startWakefulService(MwmApplication.get(), new Intent(MwmApplication.get(), TrackRecorderWakeService.class));
  }

  public synchronized static void stop()
  {
    TrackRecorderWakeService svc = getService();
    if (svc != null)
      svc.cancel();
  }
}
