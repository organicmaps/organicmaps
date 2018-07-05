package com.mapswithme.maps.maplayer.subway;

import android.app.Application;
import android.content.Context;
import android.support.annotation.NonNull;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;

public class SubwayManager
{
  @NonNull
  private final OnTransitSchemeChangedListener mSchemeChangedListener;

  public SubwayManager(@NonNull Application application) {
    mSchemeChangedListener = new OnTransitSchemeChangedListener.Default(application);
  }

  public void setEnabled(boolean isEnabled)
  {
    if (isEnabled == isEnabled())
      return;

    Framework.nativeSetTransitSchemeEnabled(isEnabled);
    Framework.nativeSaveSettingSchemeEnabled(isEnabled);
  }

  public boolean isEnabled()
  {
    return Framework.nativeIsTransitSchemeEnabled();
  }

  public void toggle()
  {
    setEnabled(!isEnabled());
  }

  public void initialize()
  {
    registerListener();
  }

  private void registerListener()
  {
    nativeAddListener(mSchemeChangedListener);
  }

  @NonNull
  public static SubwayManager from(@NonNull Context context)
  {
    MwmApplication app = (MwmApplication) context.getApplicationContext();
    return app.getSubwayManager();
  }
  private static native void nativeAddListener(@NonNull OnTransitSchemeChangedListener listener);

  private static native void nativeRemoveListener(@NonNull OnTransitSchemeChangedListener listener);
}
