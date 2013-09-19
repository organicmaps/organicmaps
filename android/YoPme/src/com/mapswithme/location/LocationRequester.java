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
import android.util.Log;

import com.mapswithme.yopme.util.BatteryHelper;
import com.mapswithme.yopme.util.BatteryHelper.BatteryLevel;

public class LocationRequester implements Handler.Callback
{
  private final static String TAG = LocationRequester.class.getSimpleName();

  protected Context mContext;
  protected LocationManager mLocationManager;
  protected Handler mDelayedEventsHandler;

  // Location settings
  protected Set<String> mEnabledProviders = new HashSet<String>();
  private long mMinDistance = 0;
  private long mMinTime     = 0;
  private boolean mIsListening = false;
  private final boolean mCancelGpsIfNotFound = true;
  private final Set<LocationListener> mListeners = new HashSet<LocationListener>();
  private final static long MAX_TIME_FOR_GPS_FIX = 15*60*1000;
  private BatteryLevel mBatteryLevel;
  // location settings

  // Handler messages
  private final static int WHAT_LOCATION_REQUEST_CANCELATION = 0x01;
  private final static int WHAT_LOCATION_REQUEST_GPS_CANCELLATION = 0x02;
  // handler massages

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
      Log.d(TAG, "No more listeners, stopping.");
    }
  }

  public void setMinDistance(long minDistance)
  {
    mMinDistance = minDistance;
  }

  public void setMinTime(long minTime)
  {
    mMinTime = minTime;
  }

  public void setUpProviders()
  {
    final float batteryLevel = BatteryHelper.getBatteryLevel(mContext);

    if (batteryLevel > BatteryHelper.BATTERY_LEVEL_LOW)
    {
      // GPS is expensive http://stackoverflow.com/a/4927117
      mEnabledProviders.add(LocationManager.GPS_PROVIDER);
    }
    if (batteryLevel > BatteryHelper.BATTERY_LEVEL_CRITICAL)
      mEnabledProviders.add(LocationManager.NETWORK_PROVIDER);
    // passive provider is free
    mEnabledProviders.add(LocationManager.PASSIVE_PROVIDER);

    Log.d(TAG, "Set up providers: " + mEnabledProviders + " at battery level: " + batteryLevel);
  }

  private PendingIntent mGpsLocationIntent;
  private PendingIntent mNetworkLocationIntent;
  private PendingIntent mPassiveLocationIntent;

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

    throw new IllegalArgumentException("WTF is " + provider + "?");
  }

  private final static String ACTION_LOCATION = ".location_request_action";
  private final static IntentFilter LOCATION_FILTER = new IntentFilter(ACTION_LOCATION);
  private final static String KEY_SOURCE = ".location_source";
  private final static String EXTRA_IS_GPS = ".is_gps";
  private final static String EXTRA_IS_NETWORK = ".is_network";
  private final static String EXTRA_IS_PASSAIVE = ".is_passive";

  private final BroadcastReceiver mLocationReciever = new BroadcastReceiver()
  {
    @Override
    public void onReceive(Context context, Intent intent)
    {
      Log.d(TAG, "Got location update from : " + intent.getStringExtra(KEY_SOURCE));


      for (final LocationListener listener: mListeners)
      {
        if (intent.hasExtra(LocationManager.KEY_LOCATION_CHANGED))
        {
          listener.onLocationChanged((Location)intent.getParcelableExtra(LocationManager.KEY_LOCATION_CHANGED));

          // if location is from GPS - do not cancel
          if (EXTRA_IS_GPS.equals(intent.getStringExtra(KEY_SOURCE)))
            mDelayedEventsHandler.removeMessages(WHAT_LOCATION_REQUEST_GPS_CANCELLATION);
        }
        // TODO: add provider status event etc.
      }
    }
  };

  private final BroadcastReceiver mBatteryReciever = new BroadcastReceiver() {

    @Override
    public void onReceive(Context context, Intent intent)
    {
      if (mIsListening)
      {
        final BatteryLevel newLevel = BatteryHelper.getBatteryLevelRange(mContext);
        Log.d(TAG, "Got battery update");

        if (newLevel != mBatteryLevel)
        {
          setUpProviders();
          stopListening();
          startListening();
          mBatteryLevel = newLevel;
          Log.d(TAG, "Changed providers list due to battery level update.");
        }
      }
    }
  };

  public LocationRequester(Context context)
  {
    mContext = context.getApplicationContext();
    mLocationManager = (LocationManager) mContext.getSystemService(Context.LOCATION_SERVICE);
    mDelayedEventsHandler = new Handler(this);

    createPendingIntents();
    setUpProviders();

    mBatteryLevel = BatteryHelper.getBatteryLevelRange(mContext);
  }

  public void startListening()
  {
    for (final String provider: mEnabledProviders)
    {
      if (mLocationManager.isProviderEnabled(provider))
      {
        mLocationManager
          .requestLocationUpdates(provider, mMinDistance, mMinDistance, getIntentForProvider(provider));
        Log.d(TAG, "Registered provider: " + provider);
      }
      else
        Log.d(TAG, "Provider disabled: " + provider);
    }

    mContext.registerReceiver(mLocationReciever, LOCATION_FILTER);
    mContext.registerReceiver(mBatteryReciever, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
    mIsListening = true;

    // GPS cancellation
    mDelayedEventsHandler.removeMessages(WHAT_LOCATION_REQUEST_GPS_CANCELLATION);
    if (mCancelGpsIfNotFound)
    {
      final Message msg = mDelayedEventsHandler.obtainMessage(WHAT_LOCATION_REQUEST_GPS_CANCELLATION);
      mDelayedEventsHandler.sendMessageDelayed(msg, MAX_TIME_FOR_GPS_FIX);
      Log.d(TAG, "Send GPS cancelation in ms:" + MAX_TIME_FOR_GPS_FIX);
    }
  }

  public void stopListening()
  {
    for (final String provider : mEnabledProviders)
    {
      mLocationManager.removeUpdates(getIntentForProvider(provider));
      Log.d(TAG, "Stopped listening to: " + provider);
    }

    if (mIsListening)
    {
      mContext.unregisterReceiver(mLocationReciever);
      mContext.unregisterReceiver(mBatteryReciever);
      mIsListening = false;
    }
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

  public void requestSingleUpdate(long delayMillis)
  {
    if (mEnabledProviders.size() > 0)
    {
      for (final String provider : mEnabledProviders)
      {
        if (mLocationManager.isProviderEnabled(provider))
          mLocationManager.requestSingleUpdate(provider, getIntentForProvider(provider));
      }

      if (delayMillis > 0)
        postRequestCancelation(delayMillis);
    }
    Log.d(TAG, "Send single update request");
  }

  public void removeUpdates(PendingIntent pi)
  {
    mLocationManager.removeUpdates(pi);
  }

  private void postRequestCancelation(long delayMillis)
  {
    final Message msg =
        mDelayedEventsHandler.obtainMessage(WHAT_LOCATION_REQUEST_CANCELATION);
    mDelayedEventsHandler.sendMessageDelayed(msg, delayMillis);
  }

  @Override
  public boolean handleMessage(Message msg)
  {
    if (msg.what == WHAT_LOCATION_REQUEST_CANCELATION)
    {
      stopListening();
      Log.d(TAG, "Removed all updates due to timeout");
      return true;
    }
    else if (msg.what == WHAT_LOCATION_REQUEST_GPS_CANCELLATION)
    {
      mLocationManager.removeUpdates(mGpsLocationIntent);
      Log.d(TAG, "Removed updates for GPS");
      return true;
    }

    return false;
  }
}
