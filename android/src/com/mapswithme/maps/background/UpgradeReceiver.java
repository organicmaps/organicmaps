package com.mapswithme.maps.background;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import com.mapswithme.maps.MwmApplication;

public class UpgradeReceiver extends BroadcastReceiver
{
  @Override
  public void onReceive(Context context, Intent intent)
  {
    MwmApplication.onUpgrade();
  }
}
