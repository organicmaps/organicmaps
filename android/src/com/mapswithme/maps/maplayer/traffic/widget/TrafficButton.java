package com.mapswithme.maps.maplayer.traffic.widget;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.Drawable;
import androidx.annotation.NonNull;
import android.view.View;
import android.widget.ImageButton;
import android.widget.RelativeLayout;

import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

public class TrafficButton
{
  @NonNull
  private final AnimationDrawable mLoadingAnim;
  @NonNull
  private final ImageButton mButton;

  public TrafficButton(@NonNull ImageButton trafficBtn)
  {
    mButton = trafficBtn;
    mLoadingAnim = getLoadingAnim(trafficBtn);

    RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) trafficBtn.getLayoutParams();
    params.setMargins(0, UiUtils.getStatusBarHeight(trafficBtn.getContext()), 0, 0);
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
    mButton.setImageResource(ThemeUtils.isNightTheme(mButton.getContext()) ? R.drawable.bg_subway_night_default
                                                                           : R.drawable.bg_subway_light_default);
  }

  void turnOn()
  {
    stopWaitingAnimation();
    mButton.setImageResource(ThemeUtils.isNightTheme(mButton.getContext()) ? R.drawable.ic_traffic_on_night
                                                                           : R.drawable.ic_traffic_on);
  }

  void markAsOutdated()
  {
    stopWaitingAnimation();
    mButton.setImageResource(ThemeUtils.isNightTheme(mButton.getContext()) ? R.drawable.ic_traffic_outdated_night
                                                                           : R.drawable.ic_traffic_outdated);
  }

  void startWaitingAnimation()
  {
    mButton.setImageDrawable(mLoadingAnim);
    AnimationDrawable anim = (AnimationDrawable) mButton.getDrawable();
    anim.start();
  }

  private void stopWaitingAnimation()
  {
    Drawable drawable = mButton.getDrawable();
    if (drawable instanceof AnimationDrawable)
    {
      AnimationDrawable animation = (AnimationDrawable) drawable;
      animation.stop();
      mButton.setImageDrawable(null);
    }
  }

  public void setOffset(int offsetX, int offsetY)
  {
    RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) mButton.getLayoutParams();
    params.setMargins(offsetX, offsetY, 0, 0);
    mButton.requestLayout();
  }

  public void show()
  {
    UiUtils.show(mButton);
  }

  public void hide()
  {
    UiUtils.hide(mButton);
  }

  public void hideImmediately()
  {
    mButton.setVisibility(View.GONE);
  }

  public void showImmediately()
  {
    mButton.setVisibility(View.VISIBLE);
  }

  public void setOnclickListener(View.OnClickListener onclickListener)
  {
    mButton.setOnClickListener(onclickListener);
  }
}
