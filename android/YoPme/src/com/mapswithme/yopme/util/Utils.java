package com.mapswithme.yopme.util;

import java.io.Closeable;
import java.io.IOException;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.text.MessageFormat;

import android.location.Location;
import android.util.Log;

public class Utils
{
  @SuppressWarnings("unchecked")
  public static <T> T createInvocationLogger(final Class<T> clazz, final String logTag)
  {
    final InvocationHandler handler = new InvocationHandler()
    {
      @Override
      public Object invoke(Object proxy, Method method, Object[] args) throws Throwable
      {
        Log.d(logTag,
            MessageFormat.format("Called {0} with {1}", method, args));

        return null;
      }
    };

    return (T) Proxy.newProxyInstance(clazz.getClassLoader(),
        new Class<?>[] { clazz }, handler);
  }

  public static void close(Closeable closeable)
  {
    if (closeable == null)
      return;

    try
    {
      closeable.close();
    }
    catch (final IOException e)
    {
      e.printStackTrace();
    }
  }

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
    final long timeDelta = (firstLoc.getElapsedRealtimeNanos() - secondLoc.getElapsedRealtimeNanos())/1000;
    final boolean isSignificantlyNewer = timeDelta > TWO_MINUTES;
    final boolean isSignificantlyOlder = timeDelta < -TWO_MINUTES;
    final boolean isNewer = timeDelta > 0;

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
    final int accuracyDelta = (int) (firstLoc.getAccuracy() - secondLoc.getAccuracy());
    // Relative diff, not absolute
    final boolean almostAsAccurate = Math.abs(accuracyDelta) <= 0.1*secondLoc.getAccuracy();

    final boolean isMoreAccurate = accuracyDelta < 0;
    final boolean isSignificantlyLessAccurate = accuracyDelta > 200;

    // Check if the old and new location are from the same provider
    final boolean isFromSameProvider = isSameProvider(firstLoc.getProvider(),
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

  private Utils() {}
}
