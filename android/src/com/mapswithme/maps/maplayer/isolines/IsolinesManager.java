package com.mapswithme.maps.maplayer.isolines;

import android.content.Context;

import androidx.annotation.NonNull;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;

public class IsolinesManager
{
  @NonNull
  public static IsolinesManager from(@NonNull Context context)
  {
    MwmApplication app = (MwmApplication) context.getApplicationContext();
    return app.getIsolinesManager();
  }

  public boolean isEnabled()
  {
    return Framework.nativeIsIsolinesLayerEnabled();
  }

  public void setEnabled(boolean isEnabled)
  {
    if (isEnabled == isEnabled())
      return;

    Framework.nativeSetIsolinesLayerEnabled(isEnabled);
  }

  public void toggle()
  {
    setEnabled(!isEnabled());
  }
}
