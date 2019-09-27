package com.mapswithme.maps.tips;

import androidx.annotation.NonNull;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.metrics.UserActionsLogger;

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
    UserActionsLogger.logTipClickedEvent(getType(), TutorialAction.ACTION_CLICKED);
    onInterceptClickInternal(activity);
  }

  public abstract void onInterceptClickInternal(@NonNull MwmActivity activity);
}
