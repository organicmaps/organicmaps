package com.mapswithme.maps.maplayer.traffic.widget;

import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;

import com.mapswithme.maps.R;
import com.mapswithme.maps.maplayer.traffic.TrafficManager;
import com.mapswithme.util.Utils;

public class TrafficButtonController implements TrafficManager.TrafficCallback
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

  public void turnOn()
  {
    mButton.turnOn();
  }

  public void hideImmediately()
  {
    mButton.hideImmediately();
  }

  public void adjust(int offsetX, int offsetY)
  {
    mButton.setOffset(offsetX, offsetY);
  }

  public void attachCore()
  {
    TrafficManager.INSTANCE.attach(this);
  }

  public void detachCore()
  {
    destroy();
  }

  public void onDisabled()
  {
    turnOff();
  }

  public void turnOff()
  {
    mButton.turnOff();
  }

  public void show()
  {
    mButton.show();
  }

  public void showImmediately()
  {
    mButton.showImmediately();
  }

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
  public void onNoData()
  {
    turnOn();
    Utils.showSnackbar(mActivity, mActivity.findViewById(R.id.coordinator),
                       mActivity.findViewById(R.id.navigation_frame),
                       R.string.traffic_data_unavailable);
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
  public void onExpiredData()
  {
    turnOn();
    Utils.showSnackbar(mActivity, mActivity.findViewById(R.id.coordinator),
                       mActivity.findViewById(R.id.navigation_frame),
                       R.string.traffic_update_maps_text);
  }

  @Override
  public void onExpiredApp()
  {
    turnOn();
    Utils.showSnackbar(mActivity, mActivity.findViewById(R.id.coordinator),
                       mActivity.findViewById(R.id.navigation_frame),
                       R.string.traffic_update_app);
  }
}
