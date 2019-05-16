package com.mapswithme.maps.tips;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.view.View;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.util.statistics.Statistics;

public abstract class TipsClickListener implements View.OnClickListener
{
  @NonNull
  private final Activity mActivity;
  @NonNull
  private final TipsApi mTipsApi;

  public TipsClickListener(@NonNull Activity activity, @NonNull TipsApi provider)
  {
    mActivity = activity;
    mTipsApi = provider;
  }

  @Override
  public final void onClick(View v)
  {
    TipsApi api = TipsApi.requestCurrent(mActivity, mActivity.getClass());
    if (api == mTipsApi)
    {
      MwmActivity mwmActivity = (MwmActivity) mActivity;
      ClickInterceptor interceptor = api.createClickInterceptor();
      interceptor.onInterceptClick(mwmActivity);
      Statistics.INSTANCE.trackTipsEvent(Statistics.EventName.TIPS_TRICKS_CLICK, api.ordinal());
      return;
    }

    onProcessClick(v);
  }

  public abstract void onProcessClick(@NonNull View view);
}
