package com.mapswithme.maps.traffic.widget;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.support.annotation.NonNull;
import android.widget.Toast;

import com.mapswithme.maps.traffic.TrafficManager;

/**
 * Created by a.zacepin on 02.12.16.
 */

public class TrafficManagerCallback implements TrafficManager.TrafficCallback
{
  @NonNull
  private final TrafficButton mButton;
  @NonNull
  private final Activity mActivity;

  public TrafficManagerCallback(@NonNull TrafficButton button, @NonNull Activity activity)
  {
    mButton = button;
    mActivity = activity;
  }

  @Override
  public void onEnabled()
  {
    mButton.turnOn();
  }

  @Override
  public void onDisabled()
  {
    mButton.turnOff();
  }

  @Override
  public void onWaitingData()
  {
    mButton.setLoading(true);
  }

  @Override
  public void onOutdated()
  {
    mButton.markAsOutdated();
  }

  @Override
  public void onNoData()
  {
    Toast.makeText(mActivity, "TODO: onNoData", Toast.LENGTH_SHORT).show();
  }

  @Override
  public void onNetworkError()
  {
    AlertDialog.Builder builder = new AlertDialog.Builder(mActivity)
        .setMessage("TODO: Show networkError dialog")
        .setPositiveButton("OK", new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            TrafficManager.INSTANCE.disable();
          }
        });
  }

  @Override
  public void onExpiredData()
  {
    Toast.makeText(mActivity, "TODO: onExpiredData", Toast.LENGTH_SHORT).show();
  }

  @Override
  public void onExpiredApp()
  {
    Toast.makeText(mActivity, "TODO: onExpiredApp", Toast.LENGTH_SHORT).show();
  }
}
