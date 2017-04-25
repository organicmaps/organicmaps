package com.mapswithme.util.permissions;

public final class PermissionsResult
{
  private final boolean mExternalStorageGranted;
  private final boolean mLocationGranted;
  private final boolean mGetAccountsGranted;

  public PermissionsResult(boolean externalStorageGranted, boolean locationGranted,
                           boolean getAccountsGranted)
  {
    mExternalStorageGranted = externalStorageGranted;
    mLocationGranted = locationGranted;
    mGetAccountsGranted = getAccountsGranted;
  }

  public boolean isExternalStorageGranted()
  {
    return mExternalStorageGranted;
  }

  public boolean isLocationGranted()
  {
    return mLocationGranted;
  }

  public boolean isGetAccountsGranted()
  {
    return mGetAccountsGranted;
  }
}
