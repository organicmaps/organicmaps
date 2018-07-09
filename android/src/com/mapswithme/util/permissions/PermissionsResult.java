package com.mapswithme.util.permissions;

public final class PermissionsResult
{
  private final boolean mExternalStorageGranted;
  private final boolean mLocationGranted;

  public PermissionsResult(boolean externalStorageGranted, boolean locationGranted)
  {
    mExternalStorageGranted = externalStorageGranted;
    mLocationGranted = locationGranted;
  }

  public boolean isExternalStorageGranted()
  {
    return mExternalStorageGranted;
  }

  public boolean isLocationGranted()
  {
    return mLocationGranted;
  }
}
