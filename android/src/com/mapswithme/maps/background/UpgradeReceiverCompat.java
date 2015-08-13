package com.mapswithme.maps.background;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class UpgradeReceiverCompat extends BroadcastReceiver
{
  @Override
  public void onReceive(Context context, Intent intent)
  {
    if (context.getPackageName().equals(intent.getData().getSchemeSpecificPart()))
      WorkerService.queuePedestrianNotification();
  }
}
