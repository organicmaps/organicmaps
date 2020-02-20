package com.mapswithme.maps.tips;

import android.app.Activity;
import android.view.View;

import androidx.annotation.NonNull;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.util.statistics.Statistics;

public abstract class TutorialClickListener implements View.OnClickListener
{
  @NonNull
  private final Activity mActivity;
  @NonNull
  private Tutorial mTutorial = Tutorial.STUB;

  public TutorialClickListener(@NonNull Activity activity)
  {
    mActivity = activity;
  }

  public void setTutorial(@NonNull Tutorial tutorial)
  {
    mTutorial = tutorial;
  }

  @Override
  public final void onClick(View v)
  {
    Tutorial tutorial = Tutorial.requestCurrent(mActivity, mActivity.getClass());
    if (tutorial == mTutorial && tutorial != Tutorial.STUB)
    {
      MwmActivity mwmActivity = (MwmActivity) mActivity;
      ClickInterceptor interceptor = tutorial.createClickInterceptor();
      interceptor.onInterceptClick(mwmActivity);
      Statistics.INSTANCE.trackTipsEvent(Statistics.EventName.TIPS_TRICKS_CLICK, tutorial.ordinal());
      return;
    }

    onProcessClick(v);
  }

  public abstract void onProcessClick(@NonNull View view);
}
