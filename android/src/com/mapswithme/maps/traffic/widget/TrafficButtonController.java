package com.mapswithme.maps.traffic.widget;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.support.annotation.NonNull;
import android.view.View;
import android.widget.Toast;

import com.mapswithme.maps.R;
import com.mapswithme.maps.traffic.TrafficManager;

public class TrafficButtonController implements TrafficManager.TrafficCallback
{
  @NonNull
  private final TrafficButton mButton;
  @NonNull
  private final Activity mActivity;
  private boolean mErrorDlgShown;

  public TrafficButtonController(@NonNull TrafficButton button, @NonNull Activity activity)
  {
    mButton = button;
    mButton.setClickListener(new OnTrafficClickListener());
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
    mButton.turnOn();
    Toast.makeText(mActivity, R.string.traffic_data_unavailable, Toast.LENGTH_SHORT).show();
  }

  @Override
  public void onNetworkError()
  {
    if (mErrorDlgShown)
      return;

    AlertDialog.Builder builder = new AlertDialog.Builder(mActivity)
        .setMessage(R.string.common_check_internet_connection_dialog)
        .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            TrafficManager.INSTANCE.disable();
            mErrorDlgShown = false;
          }
        })
        .setCancelable(true)
        .setOnCancelListener(new DialogInterface.OnCancelListener()
        {
          @Override
          public void onCancel(DialogInterface dialog)
          {
            TrafficManager.INSTANCE.disable();
            mErrorDlgShown = false;

          }
        });
    builder.show();
    mErrorDlgShown = true;
  }

  @Override
  public void onExpiredData()
  {
    mButton.turnOn();
    Toast.makeText(mActivity, R.string.traffic_update_maps_text, Toast.LENGTH_SHORT).show();
  }

  @Override
  public void onExpiredApp()
  {
    mButton.turnOn();
    Toast.makeText(mActivity, R.string.traffic_update_app, Toast.LENGTH_SHORT).show();
  }

  private class OnTrafficClickListener implements View.OnClickListener
  {
    @Override
    public void onClick(View v)
    {
      TrafficManager.INSTANCE.toggle();
    }
  }
}
