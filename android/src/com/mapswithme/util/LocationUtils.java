package com.mapswithme.util;

import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;

import android.annotation.SuppressLint;
import android.location.Location;
import android.os.SystemClock;

/**
 * Locations utils from {@link http://developer.android.com/guide/topics/location/strategies.html}
 * Partly modified and suited for MWM.
 */
public class LocationUtils
{
  private static final int TWO_MINUTES = 2 * 60 * 1000;
  private static final long LOCATION_EXPIRATION_TIME = 5 * 60 * 1000;

  @SuppressLint("NewApi")
  public static boolean isNotExpired(Location l)
  {
    if (l != null)
    {
      long timeDiff;
      if (Utils.apiEqualOrGreaterThan(17))
        timeDiff = (SystemClock.elapsedRealtimeNanos() - l.getElapsedRealtimeNanos()) / 1000000;
      else
        timeDiff = System.currentTimeMillis() - l.getTime();
      return (timeDiff <= LOCATION_EXPIRATION_TIME);
    }
    return false;
  }

  @SuppressLint("NewApi")
  public static long timeDiffMillis(Location l1, Location l2)
  {
    if (Utils.apiEqualOrGreaterThan(17))
      return (l1.getElapsedRealtimeNanos() - l2.getElapsedRealtimeNanos()) / 1000000;
    else
      return (l1.getTime() - l2.getTime());
  }

  @SuppressLint("NewApi")
  public static void setLocationCurrentTime(Location l)
  {
    if (Utils.apiEqualOrGreaterThan(17))
      l.setElapsedRealtimeNanos(SystemClock.elapsedRealtimeNanos());
    l.setTime(System.currentTimeMillis());
  }

  /// Call this function before comparing or after getting location to
  /// avoid troubles with invalid system time.
  @SuppressLint("NewApi")
  public static void hackLocationTime(Location l)
  {
    if (Utils.apiLowerThan(17))
      l.setTime(System.currentTimeMillis());
  }

  /** Determines whether one Location reading is better than the current Location fix
    * @param firstLoc  The new Location that you want to evaluate
    * @param secondLoc  The current Location fix, to which you want to compare the new one
    */
  public static boolean isFirstOneBetterLocation(Location firstLoc, Location secondLoc)
  {
    if (firstLoc == null)
      return false;
    if (secondLoc == null)
      return true;

    // Check whether the new location fix is newer or older
    final long timeDelta = timeDiffMillis(firstLoc, secondLoc);

    // If it's been more than two minutes since the current location,
    // use the new location because the user has likely moved
    if (timeDelta > TWO_MINUTES)
    {
      // significantly newer
      return true;
    }
    else if (timeDelta < -TWO_MINUTES)
    {
      // significantly older
      return false;
    }

    // Check whether the new location fix is more or less accurate
    final float accuracyDelta = firstLoc.getAccuracy() - secondLoc.getAccuracy();
    // Relative difference, not absolute
    final boolean almostAsAccurate = Math.abs(accuracyDelta) <= 0.1*secondLoc.getAccuracy();

    // Determine location quality using a combination of timeliness and accuracy
    final boolean isNewer = timeDelta > 0;
    if (accuracyDelta < 0)
    {
      // more accurate and has the same time order
      return true;
    }
    else if (isNewer && almostAsAccurate)
    {
      // newer and has the same accuracy order
      return true;
    }
    else if (isNewer && accuracyDelta <= 200 &&
             isSameProvider(firstLoc.getProvider(), secondLoc.getProvider()))
    {
      // not significantly less accurate and from the same provider
      return true;
    }

    return false;
  }

  /** Checks whether two providers are the same */
  public static boolean isSameProvider(String provider1, String provider2)
  {
    if (provider1 == null)
      return provider2 == null;
    else
      return provider1.equals(provider2);
  }

  public static Location getBestLocation(Collection<Location> locations, Comparator<Location> comparator)
  {
    return Collections.max(locations, comparator);
  }

  public static Location getMostAccurateLocation(Collection<Location> locations)
  {
    final Comparator<Location> accuracyComparator = new Comparator<Location>()
    {
      @Override
      public int compare(Location lhs, Location rhs)
      {
        return (int) (1 * Math.signum(rhs.getAccuracy() - lhs.getAccuracy()));
      }
    };

    return getBestLocation(locations, accuracyComparator);
  }

  public static Location getNewestLocation(Collection<Location> locations)
  {
    final Comparator<Location> newerFirstComparator = new Comparator<Location>()
    {
      @Override
      public int compare(Location lhs, Location rhs)
      {
        return (int) (1 * Math.signum(timeDiffMillis(lhs, rhs)));
      }
    };

    return getBestLocation(locations, newerFirstComparator);
  }


  private LocationUtils() {};
}
