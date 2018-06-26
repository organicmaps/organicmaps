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
  public SubwayManager()
  {
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
    return ThemeUtils.isNightTheme() ? R.drawable.ic_subway_menu_dark_off : R.drawable.ic_subway_menu_light_off;
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

  public static SubwayManager from(@NonNull Context context)
  {
    MwmApplication app = (MwmApplication) context.getApplicationContext();
    return app.getSubwayManager();
  }

  public void toggle()
  {
    setEnabled(!isEnabled());
  }
}
