package com.mapswithme.maps.geofence;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.NonNull;
import android.support.v4.app.JobIntentService;

import com.google.android.gms.location.Geofence;
import com.google.android.gms.location.GeofencingEvent;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationPermissionNotGrantedException;
import com.mapswithme.maps.scheduling.JobIdMap;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public class GeofenceTransitionsIntentService extends JobIntentService
{
  private static final Logger LOG = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);

  private static final String TAG = GeofenceTransitionsIntentService.class.getSimpleName();

  @NonNull
  private final Handler mMainThreadHandler = new Handler(Looper.getMainLooper());

  @Override
  protected void onHandleWork(@NonNull Intent intent)
  {
    GeofencingEvent geofencingEvent = GeofencingEvent.fromIntent(intent);
    if (geofencingEvent.hasError())
    {
      String errorMessage = "Error code = " + geofencingEvent.getErrorCode();
      LOG.e(TAG, errorMessage);
      return;
    }
    int transitionType = geofencingEvent.getGeofenceTransition();
    if (transitionType == Geofence.GEOFENCE_TRANSITION_ENTER
        || transitionType == Geofence.GEOFENCE_TRANSITION_EXIT)
    {
      mMainThreadHandler.post(new GeofencingEventTask(getApplication(), geofencingEvent));
//      LightFramework.nativeLogLocalAdsEvent(1, /* myPlaceLat */, /* myPlaceLon */, /*
// locationProviderAccuracy */, , , );
    }
  }

  public static void enqueueWork(@NonNull Context context, @NonNull Intent intent)
  {
    int id = JobIdMap.getId(GeofenceTransitionsIntentService.class);
    enqueueWork(context, GeofenceTransitionsIntentService.class, id, intent);
  }

  private static class GeofencingEventTask implements Runnable
  {
    @NonNull
    private final Application mApplication;
    @NonNull
    private final GeofencingEvent mGeofencingEvent;

    GeofencingEventTask(@NonNull Application application, @NonNull GeofencingEvent geofencingEvent)
    {
      mApplication = application;
      mGeofencingEvent = geofencingEvent;
    }

    @Override
    public void run()
    {
      Location lastKnownLocation = LocationHelper.INSTANCE.getLastKnownLocation();
      Location currentLocation = lastKnownLocation == null ? mGeofencingEvent
          .getTriggeringLocation()
                                                           : lastKnownLocation;

      GeofenceRegistry geofenceRegistry = GeofenceRegistryImpl.from(mApplication);
      if (mGeofencingEvent.getGeofenceTransition() == Geofence.GEOFENCE_TRANSITION_EXIT)
      {
        try
        {
          geofenceRegistry.unregisterGeofences();
          geofenceRegistry.registerGeofences(GeofenceLocation.from(currentLocation));
        }
        catch (LocationPermissionNotGrantedException e)
        {
          LOG.d(TAG, "Location permission not granted!", e);
        }
      }
    }
  }
}
