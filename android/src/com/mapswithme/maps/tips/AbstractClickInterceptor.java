package com.mapswithme.maps.tips;

import android.support.annotation.NonNull;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.metrics.UserActionsLogger;

public abstract class AbstractClickInterceptor implements ClickInterceptor
{
  @NonNull
  private final TipsApi mTipsApi;

  AbstractClickInterceptor(@NonNull TipsApi tipsApi)
  {
    mTipsApi = tipsApi;
  }

  @NonNull
  TipsApi getType()
  {
    return mTipsApi;
  }

  @Override
  public final void onInterceptClick(@NonNull MwmActivity activity)
  {
    UserActionsLogger.logTipsShownEvent(getType(), TipsAction.ACTION_CLICKED);
    onInterceptClickInternal(activity);
  }

  public abstract void onInterceptClickInternal(@NonNull MwmActivity activity);
}
