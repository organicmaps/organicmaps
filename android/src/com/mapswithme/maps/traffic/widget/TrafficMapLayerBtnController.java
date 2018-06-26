package com.mapswithme.maps.traffic.widget;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.widget.Toast;

import com.mapswithme.maps.R;
import com.mapswithme.maps.traffic.TrafficManager;

public class TrafficMapLayerBtnController implements TrafficManager.TrafficCallback
{
  @NonNull
  private final TrafficMapLayerButton mButton;
  @NonNull
  private final AppCompatActivity mActivity;
  @Nullable
  private Dialog mDialog;

  public TrafficMapLayerBtnController(@NonNull TrafficMapLayerButton button,
                                      @NonNull AppCompatActivity activity)
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
    mButton.startWaitingAnimation();
  }

  @Override
  public void onOutdated()
  {
    mButton.markAsOutdated();
  }

  @Override
  public void onNoData(boolean notify)
  {
    mButton.turnOn();
    if (notify)
      Toast.makeText(mActivity, R.string.traffic_data_unavailable, Toast.LENGTH_SHORT).show();
  }

  @Override
  public void onNetworkError()
  {
    if (mDialog != null && mDialog.isShowing())
      return;

    AlertDialog.Builder builder = new AlertDialog.Builder(mActivity)
        .setMessage(R.string.common_check_internet_connection_dialog)
        .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            TrafficManager.INSTANCE.disable();
          }
        })
        .setCancelable(true)
        .setOnCancelListener(new DialogInterface.OnCancelListener()
        {
          @Override
          public void onCancel(DialogInterface dialog)
          {
            TrafficManager.INSTANCE.disable();
          }
        });
    mDialog = builder.show();
  }

  public void destroy()
  {
    if (mDialog != null && mDialog.isShowing())
      mDialog.cancel();
  }

  @Override
  public void onExpiredData(boolean notify)
  {
    mButton.turnOn();
    if (notify)
      Toast.makeText(mActivity, R.string.traffic_update_maps_text, Toast.LENGTH_SHORT).show();
  }

  @Override
  public void onExpiredApp(boolean notify)
  {
    mButton.turnOn();
    if (notify)
      Toast.makeText(mActivity, R.string.traffic_update_app, Toast.LENGTH_SHORT).show();
  }
}
