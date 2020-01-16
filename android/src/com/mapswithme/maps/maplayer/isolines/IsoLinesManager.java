package com.mapswithme.maps.maplayer.isolines;

import android.content.Context;

import androidx.annotation.NonNull;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;

public class IsoLinesManager
{
  @NonNull
  public static IsoLinesManager from(@NonNull Context context)
  {
    MwmApplication app = (MwmApplication) context.getApplicationContext();
    return app.getIsoLinesManager();
  }

  public boolean isEnabled()
  {
    return Framework.nativeIsIsoLinesLayerEnabled();
  }

  public void setEnabled(boolean isEnabled)
  {
    if (isEnabled == isEnabled())
      return;

    Framework.nativeSetIsoLinesLayerEnabled(isEnabled);
  }

  public void toggle()
  {
    setEnabled(!isEnabled());
  }
}
