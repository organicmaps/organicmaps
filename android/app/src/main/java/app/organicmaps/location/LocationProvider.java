package app.organicmaps.location;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import android.app.PendingIntent;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.core.location.LocationRequestCompat;

interface LocationProvider
{
  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  void start(@NonNull LocationRequestCompat locationRequest);

  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  void stop();

  boolean isActive();

  @Nullable
  default PendingIntent getResolutionIntent()
  {
    return null;
  }
}
