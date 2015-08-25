package com.mapswithme.maps.background;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import com.mapswithme.maps.MwmApplication;

public class UpgradeReceiverCompat extends BroadcastReceiver
{
  @Override
  public void onReceive(Context context, Intent intent)
  {
    if (context.getPackageName().equals(intent.getData().getSchemeSpecificPart()))
    {
      MwmApplication.get().onUpgrade();
      // TODO uncomment after temp 5.0 release
      //    WorkerService.queuePedestrianNotification();
    }
  }
}
