package com.mapswithme.maps.location;

import android.content.Context;
import android.content.Intent;

import androidx.annotation.NonNull;

import com.mapswithme.maps.MwmBroadcastReceiver;

public class TrackRecorderWakeReceiver extends MwmBroadcastReceiver
{
  @Override
  public void onReceiveInitialized(@NonNull Context context, @NonNull Intent intent)
  {
    TrackRecorder.INSTANCE.onWakeAlarm();
  }
}
