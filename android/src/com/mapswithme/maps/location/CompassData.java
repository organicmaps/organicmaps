package com.mapswithme.maps.location;

import android.app.Activity;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.LocationUtils;


public final class CompassData
{
  private double mMagneticNorth;
  private double mTrueNorth;
  private double mNorth;

  public void update(double magneticNorth, double trueNorth)
  {
    Activity top = MwmApplication.backgroundTracker().getTopActivity();
    if (top == null)
      return;

    int rotation = top.getWindowManager().getDefaultDisplay().getRotation();
    mMagneticNorth = LocationUtils.correctCompassAngle(rotation, magneticNorth);
    mTrueNorth = LocationUtils.correctCompassAngle(rotation, trueNorth);
    mNorth = ((mTrueNorth >= 0.0) ? mTrueNorth : mMagneticNorth);
  }

  public double getMagneticNorth()
  {
    return mMagneticNorth;
  }

  public double getTrueNorth()
  {
    return mTrueNorth;
  }

  public double getNorth()
  {
    return mNorth;
  }
}

