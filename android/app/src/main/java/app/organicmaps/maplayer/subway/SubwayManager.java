package app.organicmaps.maplayer.subway;

import android.app.Application;
import android.content.Context;

import androidx.annotation.NonNull;

import app.organicmaps.Framework;
import app.organicmaps.MwmApplication;

public class SubwayManager
{
  @NonNull
  private final OnTransitSchemeChangedListener mSchemeChangedListener;

  public SubwayManager(@NonNull Application application) {
    mSchemeChangedListener = new OnTransitSchemeChangedListener.Default(application);
  }

  static public void setEnabled(boolean isEnabled)
  {
    if (isEnabled == isEnabled())
      return;

    Framework.nativeSetTransitSchemeEnabled(isEnabled);
    Framework.nativeSaveSettingSchemeEnabled(isEnabled);
  }

  static public boolean isEnabled()
  {
    return Framework.nativeIsTransitSchemeEnabled();
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
