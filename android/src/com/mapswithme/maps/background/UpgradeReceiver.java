package com.mapswithme.maps.background;

import android.content.Context;
import android.content.Intent;

import androidx.annotation.NonNull;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.MwmBroadcastReceiver;

public class UpgradeReceiver extends MwmBroadcastReceiver
{
  @Override
  protected void onReceiveInitialized(@NonNull Context context, @NonNull Intent intent)
  {
    MwmApplication.onUpgrade(context);
  }
}
