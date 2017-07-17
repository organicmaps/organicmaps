package com.mapswithme.util;

import android.support.annotation.NonNull;

import com.crashlytics.android.Crashlytics;
import com.mapswithme.maps.MwmApplication;

public final class CrashlyticsUtils
{
  public static void logException(@NonNull Throwable exception)
  {
    if (!checkCrashlytics())
      return;

    Crashlytics.logException(exception);
  }

  public static void log(int priority, @NonNull String tag, @NonNull String msg)
  {
    if (!checkCrashlytics())
      return;

    Crashlytics.log(priority, tag, msg);
  }

  private static boolean checkCrashlytics()
  {
    if (!MwmApplication.isCrashlyticsEnabled())
      return false;

    MwmApplication application = MwmApplication.get();
    if (!application.isCrashlyticsInitialized())
      application.initCrashlytics();
    return true;
  }

  private CrashlyticsUtils() {}
}
