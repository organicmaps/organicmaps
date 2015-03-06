package com.mapswithme.util;

import android.location.Location;
import android.os.Build;
import android.os.SystemClock;
import android.view.Surface;

import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.location.LocationHelper;

public class LocationUtils
{
  private LocationUtils() {}

  public static final long LOCATION_EXPIRATION_TIME_MILLIS_SHORT = 60 * 1000; // 1 minute
  public static final long LOCATION_EXPIRATION_TIME_MILLIS_LONG = 6 * 60 * 60 * 1000; // 6 hours

  public static final double LAT_LON_EPSILON = 1E-6;

  /**
   * Correct compass angles due to display orientation.
   */
  public static double correctCompassAngle(int displayOrientation, double angle)
  {
    double correction = 0;
    switch (displayOrientation)
    {
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

    // negative values (like -1.0) should remain negative (indicates that no direction available)
    if (angle >= 0.0)
      angle = correctAngle(angle, correction);

    return angle;
  }

  public static double correctAngle(double angle, double correction)
  {
    angle += correction;

    final double twoPI = 2.0 * Math.PI;
    angle = angle % twoPI;

    // normalize angle into [0, 2PI]
    if (angle < 0.0)
      angle += twoPI;

    return angle;
  }

  public static double bearingToHeading(double bearing)
  {
    return correctAngle(0.0, Math.toRadians(bearing));
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

  public static double getDiffWithLastLocation(Location lastLocation, Location newLocation)
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
        lastTime = LocationHelper.INSTANCE.getLastLocationTime();
      }

      return (time - lastTime) * 1.0E-3;
    }
  }

  public static boolean isSameLocationProvider(String p1, String p2)
  {
    return !(p1 == null || p2 == null) && p1.equals(p2);
  }

  /**
   * Detects whether object are close enough to each other, so that we can assume they represent the same poi.
   * @param first object
   * @param second object
   */
  public static boolean areLatLonEqual(MapObject first, MapObject second)
  {
    return Math.abs(first.getLat() - second.getLat()) < LocationUtils.LAT_LON_EPSILON &&
        Math.abs(first.getLon() - second.getLon()) < LocationUtils.LAT_LON_EPSILON;
  }
}
