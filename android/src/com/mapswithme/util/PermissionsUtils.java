package com.mapswithme.util;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.permissions.PermissionsResult;

import java.util.HashMap;
import java.util.Map;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.Manifest.permission.GET_ACCOUNTS;
import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;

public final class PermissionsUtils
{
  private static final String[] PERMISSIONS = new String[]
      {
          WRITE_EXTERNAL_STORAGE,
          ACCESS_COARSE_LOCATION,
          ACCESS_FINE_LOCATION,
          GET_ACCOUNTS
      };

  private PermissionsUtils() {}

  @NonNull
  public static PermissionsResult computePermissionsResult(@NonNull String[] permissions,
                                                           @NonNull int[] grantResults)
  {
    Map<String, Boolean> result = new HashMap<>();
    for (int i = 0; i < permissions.length; i++)
    {
      result.put(permissions[i], grantResults[i] == PackageManager.PERMISSION_GRANTED);
    }

    return getPermissionsResult(result);
  }

  public static boolean isLocationGranted()
  {
    return checkPermissions().isLocationGranted();
  }

  public static boolean isExternalStorageGranted()
  {
    return checkPermissions().isExternalStorageGranted();
  }

  public static boolean isGetAccountsGranted()
  {
    return checkPermissions().isGetAccountsGranted();
  }

  @NonNull
  private static PermissionsResult checkPermissions()
  {
    Context context = MwmApplication.get().getApplicationContext();
    Map<String, Boolean> result = new HashMap<>();
    for (String permission: PERMISSIONS)
    {
      result.put(permission,
                 Build.VERSION.SDK_INT < Build.VERSION_CODES.M
                 || context.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED);
    }

    return getPermissionsResult(result);
  }

  @NonNull
  private static PermissionsResult getPermissionsResult(@NonNull Map<String, Boolean> result)
  {
    boolean externalStorageGranted = result.containsKey(WRITE_EXTERNAL_STORAGE)
                                     ? result.get(WRITE_EXTERNAL_STORAGE) : false;
    boolean locationGranted = (result.containsKey(ACCESS_COARSE_LOCATION)
                               ? result.get(ACCESS_COARSE_LOCATION) : false)
                              || (result.containsKey(ACCESS_FINE_LOCATION)
                                  ? result.get(ACCESS_FINE_LOCATION) : false);
    boolean getAccountsGranted = result.containsKey(GET_ACCOUNTS)
                                 ? result.get(GET_ACCOUNTS) : false;
    return new PermissionsResult(externalStorageGranted, locationGranted, getAccountsGranted);
  }

  public static void requestPermissions(@NonNull Activity activity, int code)
  {
    ActivityCompat.requestPermissions(activity, PERMISSIONS, code);
  }
}
