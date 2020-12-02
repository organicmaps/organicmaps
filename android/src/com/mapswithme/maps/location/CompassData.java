package com.mapswithme.maps.location;

import android.app.Activity;
import android.content.Context;

import androidx.annotation.NonNull;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.LocationUtils;


public final class CompassData
{
  private double mNorth;

  public void update(@NonNull Context context, double north)
  {
    Activity top = MwmApplication.backgroundTracker(context).getTopActivity();
    if (top == null)
      return;

    int rotation = top.getWindowManager().getDefaultDisplay().getRotation();
    mNorth = LocationUtils.correctCompassAngle(rotation, north);
  }

  public double getNorth()
  {
    return mNorth;
  }
}

