package com.mapswithme.maps.maplayer.traffic.widget;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.widget.Toast;

import com.mapswithme.maps.R;
import com.mapswithme.maps.maplayer.MapLayerController;
import com.mapswithme.maps.maplayer.traffic.TrafficManager;

public class TrafficButtonController implements TrafficManager.TrafficCallback, MapLayerController
{
  @NonNull
  private final TrafficButton mButton;
  @NonNull
  private final Activity mActivity;
  @Nullable
  private Dialog mDialog;

  public TrafficButtonController(@NonNull TrafficButton button,
                                 @NonNull Activity activity)
  {
    mButton = button;
    mActivity = activity;
  }

  @Override
  public void onEnabled()
  {
    turnOn();
  }

  @Override
  public void turnOn()
  {
    mButton.turnOn();
  }

  @Override
  public void hideImmediately()
  {
    mButton.hideImmediately();
  }

  @Override
  public void adjust(int offsetX, int offsetY)
  {
    mButton.setOffset(offsetX, offsetY);
  }

  @Override
  public void attachCore()
  {
    TrafficManager.INSTANCE.attach(this);
  }

  @Override
  public void detachCore()
  {
    destroy();
  }

  @Override
  public void onDisabled()
  {
    turnOff();
  }

  @Override
  public void turnOff()
  {
    mButton.turnOff();
  }

  @Override
  public void show()
  {
    mButton.show();
  }

  @Override
  public void showImmediately()
  {
    mButton.showImmediately();
  }

  @Override
  public void hide()
  {
    mButton.hide();
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
    turnOn();
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
            TrafficManager.INSTANCE.setEnabled(false);
          }
        })
        .setCancelable(true)
        .setOnCancelListener(new DialogInterface.OnCancelListener()
        {
          @Override
          public void onCancel(DialogInterface dialog)
          {
            TrafficManager.INSTANCE.setEnabled(false);
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
    turnOn();
    if (notify)
      Toast.makeText(mActivity, R.string.traffic_update_maps_text, Toast.LENGTH_SHORT).show();
  }

  @Override
  public void onExpiredApp(boolean notify)
  {
    turnOn();
    if (notify)
      Toast.makeText(mActivity, R.string.traffic_update_app, Toast.LENGTH_SHORT).show();
  }
}
