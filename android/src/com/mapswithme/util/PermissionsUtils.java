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
import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;
import static android.support.v4.app.ActivityCompat.shouldShowRequestPermissionRationale;

public final class PermissionsUtils
{
  private static final String[] PERMISSIONS = new String[]
      {
          WRITE_EXTERNAL_STORAGE,
          ACCESS_COARSE_LOCATION,
          ACCESS_FINE_LOCATION
      };

  private static final String[] LOCATION_PERMISSIONS = new String[]
      {
          ACCESS_COARSE_LOCATION,
          ACCESS_FINE_LOCATION
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

  public static boolean isLocationGranted(@NonNull Context context)
  {
    return checkPermissions(context).isLocationGranted();
  }

  /**
   *
   * Use {@link #isLocationGranted(Context)} instead.
   */
  @SuppressWarnings("DeprecatedIsStillUsed")
  @Deprecated
  public static boolean isLocationGranted()
  {
    return checkPermissions(MwmApplication.get()).isLocationGranted();
  }

  public static boolean isLocationExplanationNeeded(@NonNull Activity activity)
  {
    return shouldShowRequestPermissionRationale(activity, ACCESS_COARSE_LOCATION)
        || shouldShowRequestPermissionRationale(activity, ACCESS_FINE_LOCATION);
  }

  public static boolean isExternalStorageGranted()
  {
    return checkPermissions(MwmApplication.get()).isExternalStorageGranted();
  }

  @NonNull
  private static PermissionsResult checkPermissions(@NonNull Context context)
  {
    Context appContext = context.getApplicationContext();
    Map<String, Boolean> result = new HashMap<>();
    for (String permission: PERMISSIONS)
    {
      result.put(permission, Build.VERSION.SDK_INT < Build.VERSION_CODES.M
                 || appContext.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED);
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
    return new PermissionsResult(externalStorageGranted, locationGranted);
  }

  public static void requestPermissions(@NonNull Activity activity, int code)
  {
    ActivityCompat.requestPermissions(activity, PERMISSIONS, code);
  }

  public static void requestLocationPermission(@NonNull Activity activity, int code)
  {
    ActivityCompat.requestPermissions(activity, LOCATION_PERMISSIONS, code);
  }
}
