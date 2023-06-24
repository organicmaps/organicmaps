package app.organicmaps.maplayer.subway;

import android.app.Application;
import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.Framework;
import app.organicmaps.MwmApplication;
import app.organicmaps.base.Initializable;

public class SubwayManager implements Initializable<Void>
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

  @Override
  public void initialize(@Nullable Void aVoid)
  {
    registerListener();
  }

  @Override
  public void destroy()
  {
    // No op.
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
