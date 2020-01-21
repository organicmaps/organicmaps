package com.mapswithme.maps.maplayer.isolines;

import android.app.Application;
import android.content.Context;

import androidx.annotation.NonNull;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.maplayer.AbstractMapLayerListener;
import com.mapswithme.maps.maplayer.OnTransitSchemeChangedListener;

public class IsolinesManager extends AbstractMapLayerListener
{
  public IsolinesManager(@NonNull Application application)
  {
    super(new IsolinesStateChangedListener(application));
  }

  @Override
  public boolean isEnabled()
  {
    return Framework.nativeIsIsolinesLayerEnabled();
  }

  @Override
  protected void setEnabledInternal(boolean isEnabled)
  {
    Framework.nativeSetIsolinesLayerEnabled(isEnabled);
  }

  @Override
  protected void registerListener()
  {
    nativeAddListener(getSchemeChangedListener());
  }

  @NonNull
  public static IsolinesManager from(@NonNull Context context)
  {
    MwmApplication app = (MwmApplication) context.getApplicationContext();
    return app.getIsolinesManager();
  }

  private static native void nativeAddListener(@NonNull OnTransitSchemeChangedListener listener);
  private static native void nativeRemoveListener(@NonNull OnTransitSchemeChangedListener listener);
}
