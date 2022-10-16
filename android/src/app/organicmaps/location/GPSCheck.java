package app.organicmaps.location;

import android.content.Context;
import android.content.Intent;
import android.location.LocationManager;
import android.text.TextUtils;

import app.organicmaps.MwmApplication;
import app.organicmaps.MwmBroadcastReceiver;

public class GPSCheck extends MwmBroadcastReceiver
{
  @Override
  public void onReceiveInitialized(Context context, Intent intent)
  {
    if (!TextUtils.equals(intent.getAction(), LocationManager.PROVIDERS_CHANGED_ACTION))
    {
      throw new AssertionError("An intent with wrong action detected: " + intent.getAction());
    }
    if (MwmApplication.backgroundTracker(context).isForeground())
    {
      LocationHelper.INSTANCE.restart();
    }
  }
}
