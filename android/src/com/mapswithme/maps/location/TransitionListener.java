package com.mapswithme.maps.location;

import android.content.Intent;
import android.content.IntentFilter;
import android.location.LocationManager;
import android.support.annotation.NonNull;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.background.AppBackgroundTracker;

class TransitionListener implements AppBackgroundTracker.OnTransitionListener
{
  @NonNull
  private final GPSCheck mReceiver = new GPSCheck();
  private boolean mReceiverRegistered;

  @Override
  public void onTransit(boolean foreground)
  {
    if (foreground && !mReceiverRegistered)
    {
      final IntentFilter filter = new IntentFilter();
      filter.addAction(LocationManager.PROVIDERS_CHANGED_ACTION);
      filter.addCategory(Intent.CATEGORY_DEFAULT);

      MwmApplication.get().registerReceiver(mReceiver, filter);
      mReceiverRegistered = true;
      return;
    }

    if (!foreground && mReceiverRegistered)
    {
      MwmApplication.get().unregisterReceiver(mReceiver);
      mReceiverRegistered = false;
    }
  }
}
