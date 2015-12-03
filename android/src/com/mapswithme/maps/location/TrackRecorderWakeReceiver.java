package com.mapswithme.maps.location;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class TrackRecorderWakeReceiver extends BroadcastReceiver
{
  @Override
  public void onReceive(Context context, Intent intent)
  {
    TrackRecorder.onWakeAlarm();
  }
}
