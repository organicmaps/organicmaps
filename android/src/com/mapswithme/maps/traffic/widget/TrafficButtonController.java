package com.mapswithme.maps.traffic.widget;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.support.annotation.NonNull;
import android.view.View;
import android.widget.Toast;

import com.mapswithme.maps.traffic.TrafficManager;

public class TrafficButtonController implements TrafficManager.TrafficCallback
{
  @NonNull
  private final TrafficButton mButton;
  @NonNull
  private final Activity mActivity;

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
    //TODO: put localized string
    Toast.makeText(mActivity, "There is not traffic data here", Toast.LENGTH_SHORT).show();
  }

  @Override
  public void onNetworkError()
  {
    //TODO: put localized string
    AlertDialog.Builder builder = new AlertDialog.Builder(mActivity)
        .setMessage("Network problem encountered. Traffic data cannot be obtained.")
        .setPositiveButton("OK", new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            TrafficManager.INSTANCE.disable();
          }
        })
        .setCancelable(false);
    builder.show();
  }

  @Override
  public void onExpiredData()
  {
    //TODO: put localized string
    Toast.makeText(mActivity, "Traffic data is outdated", Toast.LENGTH_SHORT).show();
  }

  @Override
  public void onExpiredApp()
  {
    //TODO: put localized string
    Toast.makeText(mActivity, "The app needs to be updated to get traffic data", Toast.LENGTH_SHORT).show();
  }

  private class OnTrafficClickListener implements View.OnClickListener
  {
    @Override
    public void onClick(View v)
    {
      TrafficManager.INSTANCE.enableOrDisable();
    }
  }
}
