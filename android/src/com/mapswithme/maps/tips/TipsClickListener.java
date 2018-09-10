package com.mapswithme.maps.tips;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.view.View;

import com.mapswithme.maps.MwmActivity;

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
    TipsApi api = TipsApi.requestCurrent(mActivity.getClass());
    if (api == mTipsApi)
    {
      MwmActivity mwmActivity = (MwmActivity) mActivity;
      ClickInterceptor interceptor = api.createClickInterceptor();
      interceptor.onInterceptClick(mwmActivity);
      return;
    }

    onProcessClick(v);
  }

  public abstract void onProcessClick(@NonNull View view);
}
