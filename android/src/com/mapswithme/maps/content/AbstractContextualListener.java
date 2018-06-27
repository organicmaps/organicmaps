package com.mapswithme.maps.content;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.MwmApplication;

import java.lang.ref.WeakReference;

public class AbstractContextualListener
{
  @NonNull
  private final WeakReference<MwmApplication> mApp;

  public AbstractContextualListener(@NonNull MwmApplication app)
  {
    mApp = new WeakReference<>(app);
  }

  @NonNull
  private WeakReference<MwmApplication> getAppReference()
  {
    return mApp;
  }

  @Nullable
  public MwmApplication getApp()
  {
    return getAppReference().get();
  }
}
