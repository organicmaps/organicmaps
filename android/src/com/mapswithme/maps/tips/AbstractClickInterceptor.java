package com.mapswithme.maps.tips;

import androidx.annotation.NonNull;

import com.mapswithme.maps.MwmActivity;

public abstract class AbstractClickInterceptor implements ClickInterceptor
{
  @NonNull
  private final Tutorial mTutorial;

  AbstractClickInterceptor(@NonNull Tutorial tutorial)
  {
    mTutorial = tutorial;
  }

  @NonNull
  Tutorial getType()
  {
    return mTutorial;
  }

  @Override
  public final void onInterceptClick(@NonNull MwmActivity activity)
  {
    onInterceptClickInternal(activity);
  }

  public abstract void onInterceptClickInternal(@NonNull MwmActivity activity);
}
