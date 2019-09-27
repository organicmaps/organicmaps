package com.mapswithme.maps;

import android.location.Location;
import androidx.annotation.NonNull;
import android.util.Log;

import com.mapswithme.maps.background.AppBackgroundTracker;
import com.mapswithme.maps.geofence.GeofenceLocation;
import com.mapswithme.maps.geofence.GeofenceRegistry;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationPermissionNotGrantedException;
import com.mapswithme.util.log.LoggerFactory;

class AppBaseTransitionListener implements AppBackgroundTracker.OnTransitionListener
{
  @NonNull
  private final MwmApplication mApplication;

  AppBaseTransitionListener(@NonNull MwmApplication application)
  {
    mApplication = application;
  }

  @Override
  public void onTransit(boolean foreground)
  {
    if (!foreground && LoggerFactory.INSTANCE.isFileLoggingEnabled())
    {
      Log.i(MwmApplication.TAG, "The app goes to background. All logs are going to be zipped.");
      LoggerFactory.INSTANCE.zipLogs(null);
    }

    if (foreground)
      return;

    updateGeofences();
  }

  private void updateGeofences()
  {
    Location lastKnownLocation = LocationHelper.INSTANCE.getLastKnownLocation();
    if (lastKnownLocation == null)
      return;

    GeofenceRegistry geofenceRegistry = mApplication.getGeofenceRegistry();
    try
    {
      geofenceRegistry.unregisterGeofences();
      geofenceRegistry.registerGeofences(GeofenceLocation.from(lastKnownLocation));
    }
    catch (LocationPermissionNotGrantedException e)
    {
      mApplication.getLogger().d(MwmApplication.TAG, "Location permission not granted!", e);
    }
  }
}
