package app.organicmaps.maplayer.isolines;

import android.app.Application;
import android.content.Context;

import androidx.annotation.NonNull;

import app.organicmaps.Framework;
import app.organicmaps.MwmApplication;
import app.organicmaps.util.ThemeSwitcher;
import app.organicmaps.util.ThemeUtils;

public class IsolinesManager
{
  @NonNull
  private final OnIsolinesChangedListener mListener;

  public IsolinesManager(@NonNull Application application)
  {
    mListener = new OnIsolinesChangedListener(application);
  }

  public boolean isEnabled()
  {
    return Framework.nativeIsIsolinesLayerEnabled();
  }

  private void registerListener()
  {
    nativeAddListener(mListener);
  }

  public void setEnabled(@NonNull Context context, boolean isEnabled)
  {
    if (isEnabled == isEnabled())
      return;

    Framework.nativeSetIsolinesLayerEnabled(isEnabled);
    @Framework.MapStyle
    int newMapStyle;
    if (isEnabled)
    {
      if (ThemeUtils.isDefaultTheme(context))
        newMapStyle = Framework.MAP_STYLE_OUTDOORS_CLEAR;
      else
        newMapStyle = Framework.MAP_STYLE_OUTDOORS_DARK;
    }
    else
    {
      if (ThemeUtils.isDefaultTheme(context))
        newMapStyle = Framework.MAP_STYLE_CLEAR;
      else
        newMapStyle = Framework.MAP_STYLE_DARK;
    }
    ThemeSwitcher.SetMapStyle(newMapStyle);
  }

  public void initialize()
  {
    registerListener();
  }

  @NonNull
  public static IsolinesManager from(@NonNull Context context)
  {
    MwmApplication app = (MwmApplication) context.getApplicationContext();
    return app.getIsolinesManager();
  }

  private static native void nativeAddListener(@NonNull OnIsolinesChangedListener listener);
  private static native void nativeRemoveListener(@NonNull OnIsolinesChangedListener listener);
  private static native boolean nativeShouldShowNotification();

  public void attach(@NonNull IsolinesErrorDialogListener listener)
  {
    mListener.attach(listener);
  }

  public void detach()
  {
    mListener.detach();
  }

  public boolean shouldShowNotification()
  {
    return nativeShouldShowNotification();
  }
}
