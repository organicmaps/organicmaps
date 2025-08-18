package app.organicmaps.sdk.maplayer.subway;

import android.content.Context;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.Framework;

public class SubwayManager
{
  @NonNull
  private final OnTransitSchemeChangedListener mSchemeChangedListener;

  public SubwayManager(@NonNull Context context)
  {
    mSchemeChangedListener = new OnTransitSchemeChangedListener.Default(context);
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

  private static native void nativeAddListener(@NonNull OnTransitSchemeChangedListener listener);

  private static native void nativeRemoveListener(@NonNull OnTransitSchemeChangedListener listener);
}
