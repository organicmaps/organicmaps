package com.mapswithme.location;

import android.app.PendingIntent;
import android.content.Context;
import android.location.LocationManager;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

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

  public boolean requestSingleUpdate(PendingIntent pi)
  {
    return false;
  }

  public boolean requestSingleUpdate(PendingIntent pi, long maxTimeToDeliver)
  {
    if (mEnabledProviders.length > 0)
    {
      for (final String provider : mEnabledProviders)
      {
        if (mLocationManager.isProviderEnabled(provider))
          mLocationManager.requestSingleUpdate(provider, pi);
      }

      if (maxTimeToDeliver > 0)
        postRequestCancelation(pi, maxTimeToDeliver);

      return true;
    }
    return false;
  }

  public void removeUpdates(PendingIntent pi)
  {
    mLocationManager.removeUpdates(pi);
  }

  private void postRequestCancelation(PendingIntent pi, long maxTimeToDeliver)
  {
    final Message msg =
        mDelayedEventsHandler.obtainMessage(WHAT_LOCATION_REQUEST_CANCELATION);
    msg.obj = pi;
    mDelayedEventsHandler.sendMessageDelayed(msg, maxTimeToDeliver);

    Log.d(TAG, "Posted request cancelation for " + pi + "in " + maxTimeToDeliver + "ms");
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
