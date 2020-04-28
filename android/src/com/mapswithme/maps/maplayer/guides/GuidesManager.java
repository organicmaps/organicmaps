package com.mapswithme.maps.maplayer.guides;

import android.app.Application;
import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.base.Initializable;

public class GuidesManager implements Initializable<Void>
{
  @NonNull
  private final OnGuidesChangedListener mListener;
  @NonNull
  private final Application mApplication;

  public GuidesManager(@NonNull Application application)
  {
    mApplication = application;
    mListener = this::onStateChanged;
  }

  private void onStateChanged(int index)
  {
    GuidesState state = GuidesState.values()[index];
    state.activate(mApplication);
  }

  public boolean isEnabled()
  {
    return Framework.nativeIsGuidesLayerEnabled();
  }

  private void registerListener()
  {
    nativeAddListener(mListener);
  }

  public void setEnabled(boolean isEnabled)
  {
    if (isEnabled == isEnabled())
      return;

    Framework.nativeSetGuidesLayerEnabled(isEnabled);
  }

  public void toggle()
  {
    setEnabled(!isEnabled());
  }

  @Override
  public void initialize(@Nullable Void param)
  {
    registerListener();
  }

  @Override
  public void destroy()
  {
  }

  @NonNull
  public static GuidesManager from(@NonNull Context context)
  {
    MwmApplication app = (MwmApplication) context.getApplicationContext();
    return app.getGuidesManager();
  }

  private static native void nativeAddListener(@NonNull OnGuidesChangedListener listener);
}
