package com.mapswithme.util.permissions;

public final class PermissionsResult
{
  private final boolean mLocationGranted;

  public PermissionsResult(boolean locationGranted)
  {
    mLocationGranted = locationGranted;
  }

  public boolean isLocationGranted()
  {
    return mLocationGranted;
  }
}
