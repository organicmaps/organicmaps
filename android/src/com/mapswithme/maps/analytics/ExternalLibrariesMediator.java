package com.mapswithme.maps.analytics;

import android.app.Application;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public class ExternalLibrariesMediator
{
  private static final String TAG = ExternalLibrariesMediator.class.getSimpleName();
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);

  @NonNull
  private final Application mApplication;
  @NonNull
  private volatile EventLogger mEventLogger;

  public ExternalLibrariesMediator(@NonNull Application application)
  {
    mApplication = application;
    mEventLogger = new DefaultEventLogger(application);
  }

  public boolean isCrashlyticsEnabled()
  {
    String prefKey = mApplication.getResources().getString(R.string.pref_crash_reports);
    return MwmApplication.prefs(mApplication).getBoolean(prefKey, true);
  }

  @NonNull
  public EventLogger getEventLogger()
  {
    return mEventLogger;
  }

  @Nullable
  public String retrieveFirstLaunchDeeplink()
  {
    return null;
  }

  @UiThread
  private static native void nativeInitCrashlytics();
}
