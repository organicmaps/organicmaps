package com.mapswithme.maps.traffic.widget;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.view.View;
import android.widget.ImageButton;
import android.widget.RelativeLayout;

import com.mapswithme.maps.R;
import com.mapswithme.util.Animations;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

public class TrafficMapLayerButton
{
  @NonNull
  private final AnimationDrawable mLoadingAnim;
  @NonNull
  private final ImageButton mTraffic;

  public TrafficMapLayerButton(@NonNull ImageButton trafficBtn)
  {
    mTraffic = trafficBtn;
    mLoadingAnim = getLoadingAnim(trafficBtn);

    RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) trafficBtn.getLayoutParams();
    params.setMargins(0, UiUtils.getStatusBarHeight(trafficBtn.getContext()), 0, 0);
    trafficBtn.setVisibility(View.GONE);
  }

  @NonNull
  private static AnimationDrawable getLoadingAnim(@NonNull ImageButton trafficBtn)
  {
    Context context = trafficBtn.getContext();
    Resources res = context.getResources();
    final int animResId = ThemeUtils.getResource(context, R.attr.trafficLoadingAnimation);
    return Utils.isLollipopOrLater()
           ? (AnimationDrawable) res.getDrawable(animResId, context.getTheme())
           : (AnimationDrawable) res.getDrawable(animResId);
  }

  void turnOff()
  {
    stopWaitingAnimation();
    mTraffic.setImageResource(ThemeUtils.isNightTheme() ? R.drawable.ic_traffic_off_night
                                                        : R.drawable.ic_traffic_off);
  }

  void turnOn()
  {
    stopWaitingAnimation();
    mTraffic.setImageResource(ThemeUtils.isNightTheme() ? R.drawable.ic_traffic_on_night
                                                        : R.drawable.ic_traffic_on);
  }

  void markAsOutdated()
  {
    stopWaitingAnimation();
    mTraffic.setImageResource(ThemeUtils.isNightTheme() ? R.drawable.ic_traffic_outdated_night
                                                        : R.drawable.ic_traffic_outdated);
  }

  void startWaitingAnimation()
  {
    mTraffic.setImageDrawable(mLoadingAnim);
    AnimationDrawable anim = (AnimationDrawable) mTraffic.getDrawable();
    anim.start();
  }

  private void stopWaitingAnimation()
  {
    Drawable drawable = mTraffic.getDrawable();
    if (drawable instanceof AnimationDrawable)
    {
      AnimationDrawable animation = (AnimationDrawable) drawable;
      animation.stop();
      mTraffic.setImageDrawable(null);
    }
  }

  public void setOffset(int offsetX, int offsetY)
  {
    RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) mTraffic.getLayoutParams();
    params.setMargins(offsetX, offsetY, 0, 0);
    mTraffic.requestLayout();
  }

  public void show()
  {
    Animations.appearSliding(mTraffic, Animations.LEFT, null);
  }

  public void hide()
  {
    Animations.disappearSliding(mTraffic, Animations.LEFT, null);
  }

  public void hideImmediately()
  {
    mTraffic.setVisibility(View.GONE);
  }

  public void showImmediately()
  {
    mTraffic.setVisibility(View.VISIBLE);
  }
}
