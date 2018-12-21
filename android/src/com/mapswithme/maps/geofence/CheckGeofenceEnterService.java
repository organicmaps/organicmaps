package com.mapswithme.maps.geofence;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.JobIntentService;

import com.google.android.gms.location.Geofence;
import com.google.android.gms.location.GeofencingEvent;
import com.mapswithme.maps.LightFramework;
import com.mapswithme.maps.scheduling.JobIdMap;
import com.mapswithme.util.concurrency.UiThread;

import java.util.Collections;
import java.util.List;

public class CheckGeofenceEnterService extends JobIntentService
{
  @Override
  protected void onHandleWork(@NonNull Intent intent)
  {
    GeofencingEvent geofencingEvent = intent.getParcelableExtra("my");

    GeofenceLocation geofenceLocation = GeofenceLocation.from(geofencingEvent.getTriggeringLocation());
    List<Geofence> geofences = Collections.unmodifiableList(geofencingEvent.getTriggeringGeofences());
    CheckLocationTask locationTask = new CheckLocationTask(
        getApplication(),
        geofences,
        geofenceLocation);
    UiThread.runLater(locationTask);
  }

  public static void enqueueWork(@NonNull Context context, @NonNull Intent intent)
  {
    int id = JobIdMap.getId(CheckGeofenceEnterService.class);
    enqueueWork(context, CheckGeofenceEnterService.class, id, intent);
  }

  private static class CheckLocationTask extends GeofenceTransitionsIntentService.AbstractGeofenceTask
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
//      LOG.d(TAG, "requestLocationCheck");
      GeofenceLocation geofenceLocation = getGeofenceLocation();
      if (!getApplication().arePlatformAndCoreInitialized())
        getApplication().initCore();

      GeofenceRegistry registry = GeofenceRegistryImpl.from(getApplication());
      for (Geofence each : mGeofences)
      {
        GeoFenceFeature geoFenceFeature = registry.getFeatureByGeofence(each);
        LightFramework.logLocalAdsEvent(geofenceLocation, geoFenceFeature);
      }
    }
  }
}
