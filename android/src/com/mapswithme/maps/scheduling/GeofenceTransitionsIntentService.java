package com.mapswithme.maps.scheduling;

import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.NonNull;
import android.support.v4.app.JobIntentService;

import com.google.android.gms.location.Geofence;
import com.google.android.gms.location.GeofencingEvent;
import com.mapswithme.maps.LightFramework;
import com.mapswithme.maps.location.LocationHelper;

public class GeofenceTransitionsIntentService extends JobIntentService
{
  @NonNull
  private final Handler mMainThreadHandler = new Handler(Looper.getMainLooper());

  @Override
  protected void onHandleWork(@NonNull Intent intent)
  {
    GeofencingEvent geofencingEvent = GeofencingEvent.fromIntent(intent);
    if (geofencingEvent.hasError())
      return;
    int transitionType = geofencingEvent.getGeofenceTransition();
    if (transitionType == Geofence.GEOFENCE_TRANSITION_ENTER
        || transitionType == Geofence.GEOFENCE_TRANSITION_EXIT)
    {
      mMainThreadHandler.post(new GeofencingEventTask(geofencingEvent));
//      LightFramework.nativeLogLocalAdsEvent(1, /* myPlaceLat */, /* myPlaceLon */, /* locationProviderAccuracy */, , , );
    }
  }

  public static void enqueueWork(@NonNull Context context, @NonNull Intent intent) {
    int id = JobIdMap.getId(GeofenceTransitionsIntentService.class);
    enqueueWork(context, GeofenceTransitionsIntentService.class, id, intent);
  }

  private static class GeofencingEventTask implements Runnable
  {
    @NonNull
    private final GeofencingEvent mGeofencingEvent;

    GeofencingEventTask(@NonNull GeofencingEvent geofencingEvent)
    {
      mGeofencingEvent = geofencingEvent;
    }

    @Override
    public void run()
    {
      Location lastKnownLocation = LocationHelper.INSTANCE.getLastKnownLocation();
      double accuracy = lastKnownLocation == null ? mGeofencingEvent.getTriggeringLocation().getAccuracy()
                                                  : lastKnownLocation.getAccuracy();
    }
  }
}
