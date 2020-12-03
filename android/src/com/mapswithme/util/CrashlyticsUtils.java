package com.mapswithme.util;

import android.content.Context;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.google.firebase.crashlytics.FirebaseCrashlytics;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.base.Initializable;

public enum CrashlyticsUtils implements Initializable<Context>
{
  INSTANCE;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Context mContext;

  public void logException(@NonNull Throwable exception)
  {
    if (!checkCrashlytics())
      return;

    FirebaseCrashlytics.getInstance().recordException(exception);
  }

  public void log(int priority, @NonNull String tag, @NonNull String msg)
  {
    if (!checkCrashlytics())
      return;

    FirebaseCrashlytics.getInstance().log(toLevel(priority) + "/" + tag + ": " + msg);
  }

  private boolean checkCrashlytics()
  {
    MwmApplication app = MwmApplication.from(mContext);
    return app.getMediator().isCrashlyticsEnabled();
  }

  @NonNull
  private static String toLevel(int level)
  {
    switch (level)
    {
      case Log.VERBOSE:
        return "V";
      case Log.DEBUG:
        return "D";
      case Log.INFO:
        return "I";
      case Log.WARN:
        return "W";
      case Log.ERROR:
        return "E";
      default:
        throw new IllegalArgumentException("Undetermined log level: " + level);
    }
  }

  @Override
  public void initialize(@Nullable Context context)
  {
    mContext = MwmApplication.from(context);
  }

  @Override
  public void destroy()
  {
    // No op
  }
}
