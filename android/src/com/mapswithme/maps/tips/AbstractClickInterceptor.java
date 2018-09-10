package com.mapswithme.maps.tips;

import android.support.annotation.NonNull;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;

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
    Framework.tipsShown(getType());
    onInterceptClickInternal(activity);
  }

  public abstract void onInterceptClickInternal(@NonNull MwmActivity activity);
}
