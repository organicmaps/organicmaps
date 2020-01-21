package com.mapswithme.maps.maplayer.subway;

import android.app.Application;
import android.content.Context;
import androidx.annotation.NonNull;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.maplayer.AbstractMapLayerListener;
import com.mapswithme.maps.maplayer.OnTransitSchemeChangedListener;

public class SubwayManager extends AbstractMapLayerListener
{
  public SubwayManager(@NonNull Application application) {
    super(new SubwayStateChangedListener(application));
  }

  @Override
  public boolean isEnabled() {
    return Framework.nativeIsTransitSchemeEnabled();
  }

  @Override
  protected void setEnabledInternal(boolean isEnabled) {
    Framework.nativeSetTransitSchemeEnabled(isEnabled);
    Framework.nativeSaveSettingSchemeEnabled(isEnabled);
  }

  @Override
  protected void registerListener() {
    nativeAddListener(getSchemeChangedListener());
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
