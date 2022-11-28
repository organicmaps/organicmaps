package app.organicmaps.background;

import android.content.Context;
import android.content.Intent;

import androidx.annotation.NonNull;

import app.organicmaps.MwmApplication;
import app.organicmaps.MwmBroadcastReceiver;

public class UpgradeReceiver extends MwmBroadcastReceiver
{
  @Override
  protected void onReceiveInitialized(@NonNull Context context, @NonNull Intent intent)
  {
    MwmApplication.onUpgrade(context);
  }
}
