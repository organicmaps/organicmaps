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

  @DrawableRes
  public int getIconRes()
  {
    return isEnabled() ? getEnabledStateIcon() : getDisabledStateIcon();
  }

  private int getEnabledStateIcon()
  {
    return R.drawable.ic_subway_menu_on;
  }

  private int getDisabledStateIcon()
  {
    return ThemeUtils.isNightTheme()
           ? R.drawable.ic_subway_menu_dark_off
           : R.drawable.ic_subway_menu_light_off;
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
