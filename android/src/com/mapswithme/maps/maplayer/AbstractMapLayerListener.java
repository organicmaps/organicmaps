package com.mapswithme.maps.maplayer;

import androidx.annotation.NonNull;

public abstract class AbstractMapLayerListener
{
  @NonNull
  private final OnTransitSchemeChangedListener mSchemeChangedListener;

  public AbstractMapLayerListener(@NonNull OnTransitSchemeChangedListener listener)
  {
    mSchemeChangedListener = listener;
  }

  public final void setEnabled(boolean isEnabled)
  {
    if (isEnabled == isEnabled())
      return;

    setEnabledInternal(isEnabled);
  }

  public final void toggle()
  {
    setEnabled(!isEnabled());
  }

  public final void initialize()
  {
    registerListener();
  }

  @NonNull
  protected OnTransitSchemeChangedListener getSchemeChangedListener()
  {
    return mSchemeChangedListener;
  }

  public abstract boolean isEnabled();

  protected abstract void setEnabledInternal(boolean isEnabled);

  protected abstract void registerListener();
}
