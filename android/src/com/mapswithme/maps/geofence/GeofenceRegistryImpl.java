package com.mapswithme.maps.geofence;

import android.app.Application;
import android.app.PendingIntent;
import android.content.Intent;
import androidx.annotation.NonNull;

import com.google.android.gms.location.Geofence;
import com.google.android.gms.location.GeofencingClient;
import com.google.android.gms.location.GeofencingRequest;
import com.google.android.gms.location.LocationServices;
import com.mapswithme.maps.LightFramework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.location.LocationPermissionNotGrantedException;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.TimeUnit;

public class GeofenceRegistryImpl implements GeofenceRegistry
{
  private static final int GEOFENCE_MAX_COUNT = 100;
  private static final int GEOFENCE_TTL_IN_DAYS = 3;
  private static final float PREFERRED_GEOFENCE_RADIUS = 100.0f;

  private static final Logger LOG = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = GeofenceRegistryImpl.class.getSimpleName();

  @NonNull
  private final Application mApplication;
  @NonNull
  private final GeofencingClient mGeofencingClient;

  public GeofenceRegistryImpl(@NonNull Application application)
  {
    mApplication = application;
    mGeofencingClient = LocationServices.getGeofencingClient(mApplication);
  }

  @Override
  public void registerGeofences(@NonNull GeofenceLocation location) throws LocationPermissionNotGrantedException
  {
    checkThread();
    checkPermission();

    List<GeoFenceFeature> features = LightFramework.getLocalAdsFeatures(
        location.getLat(), location.getLon(), location.getRadiusInMeters(), GEOFENCE_MAX_COUNT);

    LOG.d(TAG, "GeoFenceFeatures = " + Arrays.toString(features.toArray()));
    if (features.isEmpty())
      return;
    List<Geofence> geofences = new ArrayList<>();
    for (GeoFenceFeature each : features)
    {
      Geofence geofence = new Geofence.Builder()
          .setRequestId(each.getId().toFeatureIdString())
          .setCircularRegion(each.getLatitude(), each.getLongitude(), PREFERRED_GEOFENCE_RADIUS)
          .setExpirationDuration(TimeUnit.DAYS.toMillis(GEOFENCE_TTL_IN_DAYS))
          .setTransitionTypes(Geofence.GEOFENCE_TRANSITION_ENTER | Geofence.GEOFENCE_TRANSITION_EXIT)
          .build();
     geofences.add(geofence);
    }

    GeofencingRequest geofencingRequest = makeGeofencingRequest(geofences);
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
    mGeofencingClient.removeGeofences(makeGeofencePendingIntent())
                     .addOnSuccessListener(params -> onRemoveFailed())
                     .addOnSuccessListener(params -> onRemoveSucceeded());
  }

  private void onAddSucceeded()
  {
    LOG.d(TAG, "onAddSucceeded");
  }

  private void onAddFailed()
  {
    LOG.d(TAG, "onAddFailed");
  }

  private void onRemoveSucceeded()
  {
    LOG.d(TAG, "onRemoveSucceeded");
  }

  private void onRemoveFailed()
  {
    LOG.d(TAG, "onRemoveFailed");
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
  private GeofencingRequest makeGeofencingRequest(@NonNull List<Geofence> geofences)
  {
    GeofencingRequest.Builder builder = new GeofencingRequest.Builder();
    return builder.setInitialTrigger(GeofencingRequest.INITIAL_TRIGGER_ENTER)
                  .addGeofences(geofences)
                  .build();
  }

  @NonNull
  public static GeofenceRegistry from(@NonNull Application application)
  {
    MwmApplication app = (MwmApplication) application;
    return app.getGeofenceRegistry();
  }
}
