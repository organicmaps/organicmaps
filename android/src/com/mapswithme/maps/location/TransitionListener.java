package com.mapswithme.maps.location;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.LocationManager;
import androidx.annotation.NonNull;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.background.AppBackgroundTracker;

class TransitionListener implements AppBackgroundTracker.OnTransitionListener
{
  @NonNull
  private final GPSCheck mReceiver = new GPSCheck();
  private boolean mReceiverRegistered;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private final Context mContext;

  public TransitionListener(@NonNull Context context)
  {
    mContext = context;
  }

  @Override
  public void onTransit(boolean foreground)
  {
    if (foreground && !mReceiverRegistered)
    {
      final IntentFilter filter = new IntentFilter();
      filter.addAction(LocationManager.PROVIDERS_CHANGED_ACTION);
      filter.addCategory(Intent.CATEGORY_DEFAULT);

      MwmApplication.from(mContext).registerReceiver(mReceiver, filter);
      mReceiverRegistered = true;
      return;
    }

    if (!foreground && mReceiverRegistered)
    {
      MwmApplication.from(mContext).unregisterReceiver(mReceiver);
      mReceiverRegistered = false;
    }
  }
}
