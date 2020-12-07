package com.mapswithme.util;

import android.annotation.SuppressLint;
import android.app.Application;
import android.content.ContentResolver;
import android.content.Context;
import android.location.Location;
import android.location.LocationManager;
import android.os.Build;
import android.os.SystemClock;
import android.provider.Settings;
import androidx.annotation.NonNull;
import android.text.TextUtils;
import android.view.Surface;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.List;

public class LocationUtils
{
  private LocationUtils() {}

  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.LOCATION);
  private static final String TAG = LocationUtils.class.getSimpleName();

  /**
   * Correct compass angles due to display orientation.
   */
  public static double correctCompassAngle(int displayOrientation, double angle)
  {
    double correction = 0;
    switch (displayOrientation)
    {
    case Surface.ROTATION_0:
      return angle;
    case Surface.ROTATION_90:
      correction = Math.PI / 2.0;
      break;
    case Surface.ROTATION_180:
      correction = Math.PI;
      break;
    case Surface.ROTATION_270:
      correction = (3.0 * Math.PI / 2.0);
      break;
    }

    return correctAngle(angle, correction);
  }

  public static double correctAngle(double angle, double correction)
  {
    double res = angle + correction;

    final double twoPI = 2.0 * Math.PI;
    res %= twoPI;

    // normalize angle into [0, 2PI]
    if (res < 0.0)
      res += twoPI;

    return res;
  }

  public static boolean isExpired(Location l, long millis, long expirationMillis)
  {
    long timeDiff;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1)
      timeDiff = (SystemClock.elapsedRealtimeNanos() - l.getElapsedRealtimeNanos()) / 1000000;
    else
      timeDiff = System.currentTimeMillis() - millis;
    return (timeDiff > expirationMillis);
  }

  public static double getDiff(Location lastLocation, Location newLocation)
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1)
      return (newLocation.getElapsedRealtimeNanos() - lastLocation.getElapsedRealtimeNanos()) * 1.0E-9;
    else
    {
      long time = newLocation.getTime();
      long lastTime = lastLocation.getTime();
      if (!isSameLocationProvider(newLocation.getProvider(), lastLocation.getProvider()))
      {
        // Do compare current and previous system times in case when
        // we have incorrect time settings on a device.
        time = System.currentTimeMillis();
        lastTime = LocationHelper.INSTANCE.getSavedLocationTime();
      }

      return (time - lastTime) * 1.0E-3;
    }
  }

  private static boolean isSameLocationProvider(String p1, String p2)
  {
    return (p1 != null && p1.equals(p2));
  }

  @SuppressLint("InlinedApi")
  @SuppressWarnings("deprecation")
  public static boolean areLocationServicesTurnedOn(@NonNull Context context)
  {
    final ContentResolver resolver = context.getContentResolver();
    try
    {
      return Settings.Secure.getInt(resolver, Settings.Secure.LOCATION_MODE)
             != Settings.Secure.LOCATION_MODE_OFF;
    } catch (Settings.SettingNotFoundException e)
    {
      e.printStackTrace();
      return false;
    }
  }

  private static void logAvailableProviders(@NonNull Context context)
  {
    LocationManager locMngr = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
    List<String> providers = locMngr.getProviders(true);
    StringBuilder sb;
    if (!providers.isEmpty())
    {
      sb = new StringBuilder("Available location providers:");
      for (String provider : providers)
        sb.append(" ").append(provider);
    }
    else
    {
      sb = new StringBuilder("There are no enabled location providers!");
    }
    LOGGER.i(TAG, sb.toString());
  }

  public static boolean checkProvidersAvailability(@NonNull Context context)
  {
    LocationManager locationManager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
    if (locationManager == null)
    {
      LOGGER.e(TAG, "This device doesn't support the location service.");
      return false;
    }

    boolean networkEnabled = locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER);
    boolean gpsEnabled = locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER);
    LocationUtils.logAvailableProviders(context);
    return networkEnabled || gpsEnabled;
  }
}
