package com.mapswithme.location;

import android.app.PendingIntent;
import android.content.Context;
import android.location.Location;
import android.location.LocationManager;
import android.os.Handler;
import android.os.Message;

public class LocationRequester implements Handler.Callback
{
  private final static String TAG = LocationRequester.class.getSimpleName();

  protected Context mContext;
  protected LocationManager mLocationManager;
  protected Handler mDelayedEventsHandler;

  protected String[] mEnabledProviders = {
      LocationManager.GPS_PROVIDER,
      LocationManager.NETWORK_PROVIDER,
      LocationManager.PASSIVE_PROVIDER,
      }; // all of them are enabled by default

  private final static int WHAT_LOCATION_REQUEST_CANCELATION = 0x01;

  public LocationRequester(Context context)
  {
    mContext = context.getApplicationContext();
    mLocationManager = (LocationManager) mContext.getSystemService(Context.LOCATION_SERVICE);
    mDelayedEventsHandler = new Handler(this);
  }

  private static final int TWO_MINUTES = 1000 * 60 * 2;

  /** Checks whether two providers are the same */
  private static boolean isSameProvider(String provider1, String provider2)
  {
    if (provider1 == null)
      return provider2 == null;
    else
      return provider1.equals(provider2);
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
    long timeDelta = firstLoc.getElapsedRealtimeNanos() - secondLoc.getElapsedRealtimeNanos();
    timeDelta /= 1000;
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
    // Relative difference, not absolute
    final boolean almostAsAccurate = Math.abs(accuracyDelta) <= 0.1*secondLoc.getAccuracy();

    final boolean isMoreAccurate = accuracyDelta < 0;
    final boolean isSignificantlyLessAccurate = accuracyDelta > 200;

    // Check if the old and new location are from the same provider
    final boolean isFromSameProvider = isSameProvider(firstLoc.getProvider(), secondLoc.getProvider());

    // Determine location quality using a combination of timeliness and accuracy
    if (isMoreAccurate)
      return true;
    else if (isNewer && almostAsAccurate)
      return true;
    else if (isNewer && !isSignificantlyLessAccurate && isFromSameProvider)
      return true;

    return false;
  }

  public Location getLastLocation()
  {
    Location res = null;
    for (final String provider : mEnabledProviders)
    {
      if (mLocationManager.isProviderEnabled(provider))
      {
        final Location l = mLocationManager.getLastKnownLocation(provider);
        if (isFirstOneBetterLocation(l, res))
          res = l;
      }
    }
    return res;
  }

  public void requestSingleUpdate(PendingIntent pi, long delayMillis)
  {
    if (mEnabledProviders.length > 0)
    {
      for (final String provider : mEnabledProviders)
      {
        if (mLocationManager.isProviderEnabled(provider))
          mLocationManager.requestSingleUpdate(provider, pi);
      }

      if (delayMillis > 0)
        postRequestCancelation(pi, delayMillis);
    }
  }

  public void requestLocationUpdates(long minTime, float minDistance, PendingIntent pi)
  {
    for (final String provider : mEnabledProviders)
    {
      if (mLocationManager.isProviderEnabled(provider))
        mLocationManager.requestLocationUpdates(provider, minTime, minDistance, pi);
    }
  }

  public void removeUpdates(PendingIntent pi)
  {
    mLocationManager.removeUpdates(pi);
  }

  private void postRequestCancelation(PendingIntent pi, long delayMillis)
  {
    final Message msg =
        mDelayedEventsHandler.obtainMessage(WHAT_LOCATION_REQUEST_CANCELATION);
    msg.obj = pi;
    mDelayedEventsHandler.sendMessageDelayed(msg, delayMillis);
  }

  @Override
  public boolean handleMessage(Message msg)
  {
    if (msg.what == WHAT_LOCATION_REQUEST_CANCELATION)
    {
      final PendingIntent pi = (PendingIntent) msg.obj;
      removeUpdates(pi);
      return true;
    }
    return false;
  }
}
