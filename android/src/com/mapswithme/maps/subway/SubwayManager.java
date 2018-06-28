package com.mapswithme.maps.subway;

import android.content.Context;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;

public class SubwayManager
{
  @NonNull
  private final OnTransitSchemeChangedListener mSchemeChangedListener;

  public SubwayManager(@NonNull MwmApplication application) {
    mSchemeChangedListener = new OnTransitSchemeChangedListener.Default(application);
  }

  public void setEnabled(boolean isEnabled)
  {
    if (isEnabled == isEnabled())
      return;

    if (isEnabled)
      addSchemeChangedListener(mSchemeChangedListener);
    else
      removeSchemeChangedListener(mSchemeChangedListener);

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

  public void addSchemeChangedListener(@NonNull OnTransitSchemeChangedListener listener)
  {
    nativeAddListener(listener);
  }

  public void removeSchemeChangedListener(@NonNull OnTransitSchemeChangedListener listener)
  {
    nativeRemoveListener(listener);
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
