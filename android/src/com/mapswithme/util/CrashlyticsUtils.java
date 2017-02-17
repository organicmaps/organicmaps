package com.mapswithme.util;

import android.support.annotation.NonNull;

import com.crashlytics.android.Crashlytics;
import com.mapswithme.maps.MwmApplication;

public final class CrashlyticsUtils
{
  public static void logException(@NonNull Throwable exception)
  {
    if (MwmApplication.isCrashlyticsEnabled())
      Crashlytics.logException(exception);
  }

  private CrashlyticsUtils() {}
}
