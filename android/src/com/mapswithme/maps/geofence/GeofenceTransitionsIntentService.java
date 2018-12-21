package com.mapswithme.maps.geofence;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.os.Handler;
import android.os.Looper;
import android.os.Parcelable;
import android.support.annotation.NonNull;
import android.support.v4.app.JobIntentService;

import com.google.android.gms.location.Geofence;
import com.google.android.gms.location.GeofencingEvent;
import com.mapswithme.maps.LightFramework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationPermissionNotGrantedException;
import com.mapswithme.maps.scheduling.JobIdMap;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

public class GeofenceTransitionsIntentService extends JobIntentService
{
  private static final Logger LOG = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = GeofenceTransitionsIntentService.class.getSimpleName();
  public static final int LOCATION_PROBES_MAX_COUNT = 10;
  public static final int TIMEOUT_IN_MINUTS = 10;

  @NonNull
  private final Handler mMainThreadHandler = new Handler(Looper.getMainLooper());

  @Override
  protected void onHandleWork(@NonNull Intent intent)
  {
    GeofencingEvent geofencingEvent = GeofencingEvent.fromIntent(intent);
    if (geofencingEvent.hasError())
      onError(geofencingEvent);
    else
      onSuccess(geofencingEvent);
  }

  private void onSuccess(@NonNull GeofencingEvent geofencingEvent)
  {
    int transitionType = geofencingEvent.getGeofenceTransition();

    if (transitionType == Geofence.GEOFENCE_TRANSITION_DWELL)
      onGeofenceEnter(geofencingEvent);
    else if (transitionType == Geofence.GEOFENCE_TRANSITION_ENTER)
      onGeofenceExit(geofencingEvent);
  }

  private void onGeofenceExit(@NonNull GeofencingEvent geofencingEvent)
  {
    GeofenceLocation geofenceLocation = GeofenceLocation.from(geofencingEvent.getTriggeringLocation());
    mMainThreadHandler.post(new GeofencingEventExitTask(getApplication(), geofenceLocation));
  }

  private void onGeofenceEnter(@NonNull GeofencingEvent geofencingEvent)
  {
    makeLocationProbesBlocking(geofencingEvent);
  }

  private void makeLocationProbesBlocking(@NonNull GeofencingEvent geofencingEvent)
  {
    for (int i = 0; i < 1; i++)
    {
      try
      {
        makeSingleLocationProbOrThrow(geofencingEvent);
      }
      catch (InterruptedException| ExecutionException | TimeoutException e)
      {
        LOG.d(TAG, "error", e);
      }
    }
  }

  @NonNull
  private ExecutorService getExecutor()
  {
    MwmApplication app = (MwmApplication) getApplication();
    return app.getGeofenceProbesExecutor();
  }

  private void makeSingleLocationProbOrThrow(GeofencingEvent geofencingEvent) throws
                                                                              InterruptedException, ExecutionException, TimeoutException
  {
//    getExecutor().submit(new InfinityTask()).get(TIMEOUT_IN_MINUTS, TimeUnit.MINUTES);
    CountDownLatch countDownLatch = new CountDownLatch(10);
    InfinityTask infinityTask = new InfinityTask(countDownLatch);
    for (int i = 0; i < 10; i++)
    {
      GeofenceLocation geofenceLocation = GeofenceLocation.from(geofencingEvent.getTriggeringLocation());
      List<Geofence> geofences = Collections.unmodifiableList(geofencingEvent.getTriggeringGeofences());
      CheckLocationTask locationTask = new CheckLocationTask(
          getApplication(),
          geofences,
          geofenceLocation, infinityTask);
      mMainThreadHandler.postDelayed(locationTask, i * 1000 * 20);
    }
    countDownLatch.await();
  }

  private void onError(@NonNull GeofencingEvent geofencingEvent)
  {
    String errorMessage = "Error code = " + geofencingEvent.getErrorCode();
    LOG.e(TAG, errorMessage);
  }

  public static void enqueueWork(@NonNull Context context, @NonNull Intent intent)
  {
    int id = JobIdMap.getId(GeofenceTransitionsIntentService.class);
    enqueueWork(context, GeofenceTransitionsIntentService.class, id, intent);
  }

  private static class InfinityTask implements Callable<Object>
  {
    private static final int LATCH_COUNT = 1;
    private final CountDownLatch mCountDownLatch;

    public InfinityTask(CountDownLatch countDownLatch)
    {

      mCountDownLatch = countDownLatch;
    }

    @Override
    public Object call() throws Exception
    {
      mCountDownLatch.countDown();
      return null;
    }
  }

  private static class CheckLocationTask extends AbstractGeofenceTask
  {
    @NonNull
    private final List<Geofence> mGeofences;
    private final InfinityTask mInfinityTask;

    CheckLocationTask(@NonNull Application application, @NonNull List<Geofence> geofences,
                      @NonNull GeofenceLocation triggeringLocation, InfinityTask infinityTask)
    {
      super(application, triggeringLocation);
      mGeofences = geofences;
      mInfinityTask = infinityTask;
    }

    @Override
    public void run()
    {
      requestLocationCheck();
    }

    private void requestLocationCheck()
    {

      String errorMessage = "Geo = " + Arrays.toString(mGeofences.toArray());
      LOG.e(TAG, errorMessage);

      GeofenceLocation geofenceLocation = getGeofenceLocation();
      if (!getApplication().arePlatformAndCoreInitialized())
        getApplication().initCore();

      GeofenceRegistry registry = GeofenceRegistryImpl.from(getApplication());
      for (Geofence each : mGeofences)
      {
        //GeoFenceFeature geoFenceFeature = registry.getFeatureByGeofence(each);
//        LightFramework.logLocalAdsEvent(geofenceLocation, geoFenceFeature);
      }

      getApplication().getGeofenceProbesExecutor().submit(mInfinityTask);
    }
  }

  private class GeofencingEventExitTask extends AbstractGeofenceTask
  {
    GeofencingEventExitTask(@NonNull Application application,
                            @NonNull GeofenceLocation location)
    {
      super(application, location);
    }

    @Override
    public void run()
    {
      GeofenceLocation location = getGeofenceLocation();
      GeofenceRegistry geofenceRegistry = GeofenceRegistryImpl.from(getApplication());

      try
      {
        geofenceRegistry.unregisterGeofences();
        geofenceRegistry.registerGeofences(location);
      }
      catch (LocationPermissionNotGrantedException e)
      {
        LOG.d(TAG, "Location permission not granted!", e);
      }
    }
  }

  public static abstract class AbstractGeofenceTask implements Runnable
  {
    @NonNull
    private final MwmApplication mApplication;
    @NonNull
    private final GeofenceLocation mGeofenceLocation;

    AbstractGeofenceTask(@NonNull Application application,
                         @NonNull GeofenceLocation location)
    {
      mApplication = (MwmApplication)application;
      mGeofenceLocation = location;
    }

    @NonNull
    protected MwmApplication getApplication()
    {
      return mApplication;
    }

    @NonNull
    protected GeofenceLocation getGeofenceLocation()
    {
      Location lastKnownLocation = LocationHelper.INSTANCE.getLastKnownLocation();
      return lastKnownLocation == null ? mGeofenceLocation
                                       : GeofenceLocation.from(lastKnownLocation);
    }
  }
}
