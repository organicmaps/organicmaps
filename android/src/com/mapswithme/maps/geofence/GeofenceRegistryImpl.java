package com.mapswithme.maps.geofence;

import android.app.Application;
import android.app.PendingIntent;
import android.content.Intent;
import android.support.annotation.NonNull;

import com.google.android.gms.location.Geofence;
import com.google.android.gms.location.GeofencingClient;
import com.google.android.gms.location.GeofencingRequest;
import com.google.android.gms.location.LocationServices;
import com.mapswithme.maps.LightFramework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.location.LocationPermissionNotGrantedException;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.concurrency.UiThread;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

public class GeofenceRegistryImpl implements GeofenceRegistry
{
  private static final int GEOFENCE_MAX_COUNT = 100;
  private static final float PREFERRED_GEOFENCE_RADIUS = 100.0f;

  @NonNull
  private final Application mApplication;
  @NonNull
  private final List<GeofenceAndFeature> mGeofences;
  @NonNull
  private final GeofencingClient mGeofencingClient;

  public GeofenceRegistryImpl(@NonNull Application application)
  {
    mGeofences = new ArrayList<>();
    mApplication = application;
    mGeofencingClient = LocationServices.getGeofencingClient(mApplication);
  }

  @Override
  public void registerGeofences(@NonNull GeofenceLocation location) throws LocationPermissionNotGrantedException
  {
    checkThread();
    checkPermission();

    List<GeoFenceFeature> features = LightFramework.getLocalAdsFeatures(
        location.getLat(), location.getLon(), location.getRadiusInMeters()/* from system  location provider accuracy */, GEOFENCE_MAX_COUNT);

    if (features.isEmpty())
      return;
    for (GeoFenceFeature each : features)
    {
      Geofence geofence = new Geofence.Builder()
          .setRequestId(each.getId())
          .setCircularRegion(each.getLatitude(), each.getLongitude(), PREFERRED_GEOFENCE_RADIUS)
          .setExpirationDuration(Geofence.NEVER_EXPIRE)
          .setTransitionTypes(Geofence.GEOFENCE_TRANSITION_ENTER
                              | Geofence.GEOFENCE_TRANSITION_EXIT)
          .build();
      mGeofences.add(new GeofenceAndFeature(geofence, each));
    }
    GeofencingRequest geofencingRequest = makeGeofencingRequest();
    PendingIntent intent = makeGeofencePendingIntent();
    mGeofencingClient.addGeofences(geofencingRequest, intent)
                     .addOnSuccessListener(params -> onAddSucceeded())
                     .addOnFailureListener(params -> onAddFailed());
  }

  @Override
  public void unregisterGeofences() throws LocationPermissionNotGrantedException
  {
    checkThread();
    checkPermission();

    if (mGeofences.isEmpty())
      return;
    List<String> expiredGeofences = new ArrayList<>();
    for (GeofenceAndFeature current: mGeofences)
    {
      String requestId = current.getGeofence().getRequestId();
      expiredGeofences.add(requestId);
    }
    mGeofencingClient.removeGeofences(expiredGeofences)
                     .addOnSuccessListener(params -> onRemoveFailed())
                     .addOnSuccessListener(params -> onRemoveSucceeded());
  }

  private void onAddSucceeded()
  {

  }

  private void onAddFailed()
  {

  }

  private void onRemoveSucceeded()
  {

  }

  private void onRemoveFailed()
  {

  }

  private void checkPermission() throws LocationPermissionNotGrantedException
  {
    if (!PermissionsUtils.isLocationGranted(mApplication))
      throw new LocationPermissionNotGrantedException();
  }

  private static void checkThread()
  {
    if (!UiThread.isUiThread())
      throw new IllegalStateException("Must be call from Ui thread");
  }

  @NonNull
  private PendingIntent makeGeofencePendingIntent()
  {
    Intent intent = new Intent(mApplication, GeofenceReceiver.class);
    return PendingIntent.getBroadcast(mApplication, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
  }

  @NonNull
  private GeofencingRequest makeGeofencingRequest()
  {
    GeofencingRequest.Builder builder = new GeofencingRequest.Builder();
    return builder.setInitialTrigger(GeofencingRequest.INITIAL_TRIGGER_DWELL)
                  .addGeofences(collectGeofences())
                  .build();
  }

  @NonNull
  private List<Geofence> collectGeofences()
  {
    List<Geofence> geofences = new ArrayList<>();
    for (GeofenceAndFeature each : mGeofences)
    {
      geofences.add(each.getGeofence());
    }
    return geofences;
  }

  @NonNull
  public static GeofenceRegistry from(@NonNull Application application)
  {
    MwmApplication app = (MwmApplication) application;
    return app.getGeofenceRegistry();
  }

  private static class GeofenceAndFeature
  {
    @NonNull
    private final Geofence mGeofence;
    @NonNull
    private final GeoFenceFeature mFeature;

    private GeofenceAndFeature(@NonNull Geofence geofence, @NonNull GeoFenceFeature feature)
    {
      mGeofence = geofence;
      mFeature = feature;
    }

    @NonNull
    public Geofence getGeofence()
    {
      return mGeofence;
    }

    @NonNull
    public GeoFenceFeature getFeature()
    {
      return mFeature;
    }
  }
}
