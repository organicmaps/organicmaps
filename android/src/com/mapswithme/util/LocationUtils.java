package com.mapswithme.util;

import android.location.Location;
import android.os.Build;
import android.os.SystemClock;
import android.view.Surface;

public class LocationUtils
{
  private LocationUtils() {}

  public static final long LOCATION_EXPIRATION_TIME_MILLIS_SHORT = 60 * 1000; // 1 minute
  public static final long LOCATION_EXPIRATION_TIME_MILLIS_LONG = 6 * 60 * 60 * 1000; // 6 hours

  /**
   * Correct compass angles due to display orientation.
   */
  public static void correctCompassAngles(int displayOrientation, double angles[])
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

    for (int i = 0; i < angles.length; ++i)
    {
      if (angles[i] >= 0.0)
      {
        // negative values (like -1.0) should remain negative (indicates that no direction available)
        angles[i] = correctAngle(angles[i], correction);
      }
    }
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
}
