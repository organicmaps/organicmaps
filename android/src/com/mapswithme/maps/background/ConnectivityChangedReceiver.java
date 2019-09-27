package com.mapswithme.maps.background;

import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import androidx.annotation.NonNull;

public class ConnectivityChangedReceiver extends AbstractLogBroadcastReceiver
{

  @Override
  public void onReceiveInternal(@NonNull Context context, @NonNull Intent intent)
  {
    NotificationService.startOnConnectivityChanged(context);
  }

  @NonNull
  @Override
  protected String getAssertAction()
  {
    return ConnectivityManager.CONNECTIVITY_ACTION;
  }
}
