package com.mapswithme.util;

import android.location.Location;

import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;

/**
 * Locations utils from {@link http://developer.android.com/guide/topics/location/strategies.html}
 * Partly modified and suited for MWM.
 */
public class LocationUtils
{
  private static final int TWO_MINUTES = 1000 * 60 * 2;

  /** Determines whether one Location reading is better than the current Location fix
    * @param firstLoc  The new Location that you want to evaluate
    * @param secondLoc  The current Location fix, to which you want to compare the new one
    */
  public static boolean isFirstOneBetterLocation(Location firstLoc, Location secondLoc)
  {
    if (secondLoc == null)
    {
        // A new location is always better than no location
      return true;
    }

    // Check whether the new location fix is newer or older
    long timeDelta = firstLoc.getTime() - secondLoc.getTime();
    boolean isSignificantlyNewer = timeDelta > TWO_MINUTES;
    boolean isSignificantlyOlder = timeDelta < -TWO_MINUTES;
    boolean isNewer = timeDelta > 0;

    // If it's been more than two minutes since the current location, use the new location
    // because the user has likely moved
    if (isSignificantlyNewer)
    {
      return true;
    // If the new location is more than two minutes older, it must be worse
    }
    else if (isSignificantlyOlder)
      return false;

    // Check whether the new location fix is more or less accurate
    int accuracyDelta = (int) (firstLoc.getAccuracy() - secondLoc.getAccuracy());
    // Relative diff, not absolute
    boolean almostAsAccurate = Math.abs(accuracyDelta) <= 0.1*secondLoc.getAccuracy();

    boolean isMoreAccurate = accuracyDelta < 0;
    boolean isSignificantlyLessAccurate = accuracyDelta > 200;

    // Check if the old and new location are from the same provider
    boolean isFromSameProvider = isSameProvider(firstLoc.getProvider(),
                                                secondLoc.getProvider());

    // Determine location quality using a combination of timeliness and accuracy
    if (isMoreAccurate)
      return true;
    else if (isNewer && almostAsAccurate)
      return true;
    else if (isNewer && !isSignificantlyLessAccurate && isFromSameProvider)
      return true;
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
        final float deltaAcc = rhs.getAccuracy() - lhs.getAccuracy();
        return (int) (1 * Math.signum(deltaAcc));
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
        final long deltaTime = lhs.getTime() - rhs.getTime();
        return (int) (1 * Math.signum(deltaTime));
      }
    };

    return getBestLocation(locations, newerFirstComparator);
  }


  private LocationUtils() {};
}
