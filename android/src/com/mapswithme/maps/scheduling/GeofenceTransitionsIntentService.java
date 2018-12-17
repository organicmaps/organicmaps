package com.mapswithme.maps.scheduling;

import android.content.Context;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.JobIntentService;

import com.google.android.gms.location.Geofence;
import com.google.android.gms.location.GeofencingEvent;
import com.mapswithme.maps.LightFramework;

public class GeofenceTransitionsIntentService extends JobIntentService
{
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

      Geofence geofence = geofencingEvent.getTriggeringGeofences().get(0);


//      LightFramework.nativeLogLocalAdsEvent(1, /* myPlaceLat */, /* myPlaceLon */, /* locationProviderAccuracy */, , , );
    }
  }

  /**
   * Convenience method for enqueuing work in to this service.
   */
  public static void enqueueWork(Context context, Intent intent) {
    int id = JobIdMap.getId(GeofenceTransitionsIntentService.class);
    enqueueWork(context, GeofenceTransitionsIntentService.class, id, intent);
  }

}
