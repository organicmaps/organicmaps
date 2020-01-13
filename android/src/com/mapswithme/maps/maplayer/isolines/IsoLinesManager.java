package com.mapswithme.maps.maplayer.isolines;

import android.app.Application;
import android.content.Context;

import androidx.annotation.NonNull;
import com.mapswithme.maps.MwmApplication;

public class IsoLinesManager
{
  @NonNull
  private final Application mApp;
  private boolean mEnabled;

  public IsoLinesManager(@NonNull Application application)
  {
    mApp = application;
  }

  @NonNull
  public static IsoLinesManager from(@NonNull Context context)
  {
    MwmApplication app = (MwmApplication) context.getApplicationContext();
    return app.getIsoLinesManager();
  }

  public boolean isEnabled()
  {
    return mEnabled;
  }

  public void setEnabled(boolean isEnabled)
  {
    mEnabled = isEnabled;
  }

  public void toggle()
  {
    setEnabled(!isEnabled());
  }
}
