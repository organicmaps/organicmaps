package com.mapswithme.location;

import java.util.HashSet;
import java.util.Set;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Handler;
import android.os.Message;

import com.mapswithme.util.log.Logger;
import com.mapswithme.yopme.util.BatteryHelper;
import com.mapswithme.yopme.util.BatteryHelper.BatteryLevel;

public class LocationRequester implements Handler.Callback
{
  protected Context mContext;
  protected LocationManager mLocationManager;
  protected Handler mDelayedEventsHandler;

  private final Logger mLogger = StubLogger.get(); // SimpleLogger.get("MWM_DBG");

  // Location
  private boolean mIsRegistered = false;
  private BatteryLevel mBatteryLevel;
  private final Set<LocationListener> mListeners = new HashSet<LocationListener>();

  private Location mLocation = null;
  private final static long MAX_TIME_FOR_SUBSCRIPTION_FIX = 15*60*1000;

  // Handler messages
  private final static int WHAT_LOCATION_REQUEST_CANCELATION = 0x01;

  // Intents
  private PendingIntent mGpsLocationIntent;
  private PendingIntent mNetworkLocationIntent;
  private PendingIntent mPassiveLocationIntent;

  private final static String ACTION_LOCATION = ".location_request_action";
  private final static String KEY_SOURCE = ".location_source";
  private final static String EXTRA_IS_GPS = ".is_gps";
  private final static String EXTRA_IS_NETWORK = ".is_network";
  private final static String EXTRA_IS_PASSAIVE = ".is_passive";

  // Receivers
  private final BroadcastReceiver mLocationReciever = new BroadcastReceiver()
  {
    @Override
    public void onReceive(Context context, Intent intent)
    {
      mLogger.d("Got location update from : " + intent.getStringExtra(KEY_SOURCE));

      for (final LocationListener listener: mListeners)
      {
        if (intent.hasExtra(LocationManager.KEY_LOCATION_CHANGED))
        {
          final Location l = (Location)intent.getParcelableExtra(LocationManager.KEY_LOCATION_CHANGED);
          final boolean isLocationReallyChanged = isFirstOneBetterLocation(l, mLocation)
                                                  && areLocationsFarEnough(l, mLocation);
          if (isLocationReallyChanged)
          {
            mLogger.d("Old location: ", mLocation);
            mLogger.d("New location: ", l);

            listener.onLocationChanged(l);
            mLocation = l;
            postRequestCancelation(MAX_TIME_FOR_SUBSCRIPTION_FIX);
          }
        }

        // TODO: add another events processing
      }
    }
  };

  private boolean canListenToLocation()
  {
    return mBatteryLevel != BatteryLevel.CRITICAL;
  }

  private final BroadcastReceiver mBatteryReciever = new BroadcastReceiver()
  {
    @Override
    public void onReceive(Context context, Intent intent)
    {
      mBatteryLevel = BatteryHelper.getBatteryLevelRange(mContext);
      mLogger.d("Got battery update, level: " + mBatteryLevel);

      if (!canListenToLocation())
      {
        stopListening();
        mLogger.d("Stopped updated due to battery level update.");
      }
    }
  };

  public LocationRequester(Context context)
  {
    mContext = context.getApplicationContext();
    mLocationManager = (LocationManager) mContext.getSystemService(Context.LOCATION_SERVICE);
    mDelayedEventsHandler = new Handler(this);

    createPendingIntents();
    mBatteryLevel = BatteryHelper.getBatteryLevelRange(mContext);
  }

  private void createPendingIntents()
  {
    mGpsLocationIntent = PendingIntent.getBroadcast(mContext, 0xF,
        new Intent(ACTION_LOCATION).putExtra(KEY_SOURCE, EXTRA_IS_GPS), PendingIntent.FLAG_CANCEL_CURRENT);

    mNetworkLocationIntent = PendingIntent.getBroadcast(mContext, 0xFF,
        new Intent(ACTION_LOCATION).putExtra(KEY_SOURCE, EXTRA_IS_NETWORK), PendingIntent.FLAG_CANCEL_CURRENT);

    mPassiveLocationIntent = PendingIntent.getBroadcast(mContext, 0xFFF,
        new Intent(ACTION_LOCATION).putExtra(KEY_SOURCE, EXTRA_IS_PASSAIVE), PendingIntent.FLAG_CANCEL_CURRENT);
  }

  public PendingIntent getIntentForProvider(String provider)
  {
    if (LocationManager.GPS_PROVIDER.equals(provider))
      return mGpsLocationIntent;

    if (LocationManager.NETWORK_PROVIDER.equals(provider))
      return mNetworkLocationIntent;

    if (LocationManager.PASSIVE_PROVIDER.equals(provider))
      return mPassiveLocationIntent;

    return null;
  }

  public void startListening(long time, boolean isSingle)
  {
    stopListening();

    if (!canListenToLocation())
      return;

    for (final String provider: mLocationManager.getProviders(true))
    {
      final PendingIntent pi = getIntentForProvider(provider);
      if (pi == null)
        continue;

      if (isSingle)
      {
        mLocationManager.requestSingleUpdate(provider, pi);
        postRequestCancelation(time);
      }
      else
      {
        mLocationManager.requestLocationUpdates(provider, time, 0, pi);
        postRequestCancelation(MAX_TIME_FOR_SUBSCRIPTION_FIX);
      }

      mLogger.d("Registered provider: " + provider);
    }

  }

  public void stopListening()
  {
    for (final String provider : mLocationManager.getAllProviders())
    {
      final PendingIntent pi = getIntentForProvider(provider);
      if (pi != null)
      {
        mLocationManager.removeUpdates(pi);
        mLogger.d("Stopped listening to: " + provider);
      }
    }
  }

  public void unregister()
  {
    if (mIsRegistered)
    {
      mContext.unregisterReceiver(mLocationReciever);
      mContext.unregisterReceiver(mBatteryReciever);

      mIsRegistered = false;
    }
  }

  public void register()
  {
    mContext.registerReceiver(mLocationReciever, new IntentFilter(ACTION_LOCATION));
    mContext.registerReceiver(mBatteryReciever,  new IntentFilter(Intent.ACTION_BATTERY_CHANGED));

    mIsRegistered = true;
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
    timeDelta /= 1000000;
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

  static private native boolean areLocationsFarEnough(double lat1, double lon1, double lat2, double lon2);

  private boolean areLocationsFarEnough(Location l1, Location l2)
  {
    if (l1 == null || l2 == null)
      return true;

    return areLocationsFarEnough(l1.getLatitude(), l1.getLongitude(),
                                 l2.getLatitude(), l2.getLongitude());
  }

  public Location getLastKnownLocation()
  {
    Location res = null;
    for (final String provider : mLocationManager.getProviders(true))
    {
      final Location l = mLocationManager.getLastKnownLocation(provider);
      if (isFirstOneBetterLocation(l, res))
        res = l;
    }

    // Save returning location to suppress useless drawings.
    mLocation = res;
    return res;
  }

  private void postRequestCancelation(long delayMillis)
  {
    mDelayedEventsHandler.removeMessages(WHAT_LOCATION_REQUEST_CANCELATION);
    final Message msg = mDelayedEventsHandler.obtainMessage(WHAT_LOCATION_REQUEST_CANCELATION);
    mDelayedEventsHandler.sendMessageDelayed(msg, delayMillis);

    mLogger.d("Postponed cancelation in: " + delayMillis + " ms");
  }

  @Override
  public boolean handleMessage(Message msg)
  {
    if (msg.what == WHAT_LOCATION_REQUEST_CANCELATION)
    {
      stopListening();
      mLogger.d("Removed all updates due to timeout");
      return true;
    }
    return false;
  }

  public void addListener(LocationListener listener)
  {
    mListeners.add(listener);
  }

  public void removeListener(LocationListener listener)
  {
    mListeners.remove(listener);
    if (mListeners.isEmpty())
    {
      stopListening();
      mLogger.d("No more listeners, stopping.");
    }
  }

  public void setLocation(Location location)
  {
    mLocation = location;
  }
}
