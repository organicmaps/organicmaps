package com.mapswithme.maps.geofence;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.os.Handler;
import android.os.Looper;
import androidx.annotation.NonNull;
import androidx.core.app.JobIntentService;

import com.google.android.gms.location.Geofence;
import com.google.android.gms.location.GeofencingEvent;
import com.mapswithme.maps.LightFramework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationPermissionNotGrantedException;
import com.mapswithme.maps.scheduling.JobIdMap;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class GeofenceTransitionsIntentService extends JobIntentService
{
  private static final Logger LOG = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = GeofenceTransitionsIntentService.class.getSimpleName();
  private static final int LOCATION_PROBES_MAX_COUNT = 10;

  @NonNull
  private final Handler mMainThreadHandler = new Handler(Looper.getMainLooper());

  @Override
  protected void onHandleWork(@NonNull Intent intent)
  {
    LOG.d(TAG, "onHandleWork. Intent = " + intent);
    GeofencingEvent geofencingEvent = GeofencingEvent.fromIntent(intent);;
    if (geofencingEvent.hasError())
      onError(geofencingEvent);
    else
      onSuccess(geofencingEvent);
  }

  private void onSuccess(@NonNull GeofencingEvent geofencingEvent)
  {
    int transitionType = geofencingEvent.getGeofenceTransition();
    LOG.d(TAG, "transitionType = " + transitionType);

    if (transitionType == Geofence.GEOFENCE_TRANSITION_ENTER)
      onGeofenceEnter(geofencingEvent);
    else if (transitionType == Geofence.GEOFENCE_TRANSITION_EXIT)
      onGeofenceExit(geofencingEvent);
  }

  private void onGeofenceExit(@NonNull GeofencingEvent geofencingEvent)
  {
    GeofenceLocation geofenceLocation = GeofenceLocation.from(geofencingEvent.getTriggeringLocation());
    mMainThreadHandler.post(new GeofencingEventExitTask(getApplication(), geofenceLocation));
  }

  private void onGeofenceEnter(@NonNull GeofencingEvent geofencingEvent)
  {
    makeLocationProbesBlockingSafely(geofencingEvent);
  }

  private void makeLocationProbesBlockingSafely(@NonNull GeofencingEvent geofencingEvent)
  {
    try
    {
      makeLocationProbesBlocking(geofencingEvent);
    }
    catch (InterruptedException e)
    {
      LOG.e(TAG, "Failed to make location probe for '" + geofencingEvent + "'", e);
    }
  }

  private void makeLocationProbesBlocking(@NonNull GeofencingEvent event) throws
                                                                          InterruptedException
  {
    CountDownLatch latch = new CountDownLatch(LOCATION_PROBES_MAX_COUNT);
    for (int i = 0; i < LOCATION_PROBES_MAX_COUNT; i++)
    {
      makeSingleLocationProbe(event, i);
    }
    latch.await(LOCATION_PROBES_MAX_COUNT, TimeUnit.MINUTES);
  }

  private void makeSingleLocationProbe(@NonNull GeofencingEvent event, int timeoutInMinutes)
  {
    GeofenceLocation geofenceLocation = GeofenceLocation.from(event.getTriggeringLocation());
    List<Geofence> geofences = Collections.unmodifiableList(event.getTriggeringGeofences());
    CheckLocationTask locationTask = new CheckLocationTask(
        getApplication(),
        geofences,
        geofenceLocation);
    mMainThreadHandler.postDelayed(locationTask, TimeUnit.MINUTES.toMillis(timeoutInMinutes));
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
    LOG.d(TAG, "Service was enqueued");
  }

  private static class CheckLocationTask extends AbstractGeofenceTask
  {
    @NonNull
    private final List<Geofence> mGeofences;

    CheckLocationTask(@NonNull Application application, @NonNull List<Geofence> geofences,
                      @NonNull GeofenceLocation triggeringLocation)
    {
      super(application, triggeringLocation);
      mGeofences = geofences;
    }

    @Override
    public void run()
    {
      requestLocationCheck();
    }

    private void requestLocationCheck()
    {
      // Framework and LightFramework use the same files to process local ads
      // events and are not explicitly synchronized. Logging local ads events
      // from both frameworks at once may lead to a data race.
      if (getApplication().arePlatformAndCoreInitialized())
        return;

      LOG.d(TAG, "Geofences = " + Arrays.toString(mGeofences.toArray()));

      GeofenceLocation geofenceLocation = getGeofenceLocation();
      for (Geofence each : mGeofences)
      {
        FeatureId feature = Factory.from(each);
        LightFramework.logLocalAdsEvent(geofenceLocation, feature);
      }
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
      LOG.d(TAG, "Exit event for location = " + location);
      try
      {
        geofenceRegistry.unregisterGeofences();
        geofenceRegistry.registerGeofences(location);
      }
      catch (LocationPermissionNotGrantedException e)
      {
        LOG.e(TAG, "Location permission not granted!", e);
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
