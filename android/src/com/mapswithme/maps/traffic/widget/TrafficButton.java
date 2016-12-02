package com.mapswithme.maps.traffic.widget;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.AnimationDrawable;
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

public class TrafficButton
{
  @NonNull
  private final AnimationDrawable mLoadingAnim;
  @NonNull
  private final ImageButton mButton;

  public TrafficButton(@NonNull Context context, @NonNull ImageButton button)
  {
    mButton = button;
    Resources rs = context.getResources();
    @DrawableRes
    final int loadingAnimRes = ThemeUtils.getResource(context, R.attr.trafficLoadingAnimation);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
      mLoadingAnim = (AnimationDrawable) rs.getDrawable(loadingAnimRes, context.getTheme());
    else
      mLoadingAnim = (AnimationDrawable) rs.getDrawable(loadingAnimRes);

    RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) button.getLayoutParams();
    params.setMargins(0, UiUtils.getStatusBarHeight(context), 0, 0);
    button.setVisibility(View.VISIBLE);
  }

  void setClickListener(@NonNull View.OnClickListener clickListener)
  {
    mButton.setOnClickListener(clickListener);
  }

  void turnOff()
  {
    mButton.setImageResource(ThemeUtils.isNightTheme() ? R.drawable.ic_traffic_off_night
                                                       : R.drawable.ic_traffic_off);
  }

  void turnOn()
  {
    mButton.setImageResource(ThemeUtils.isNightTheme() ? R.drawable.ic_traffic_on_night
                                                       : R.drawable.ic_traffic_on);
  }

  void markAsOutdated()
  {
    mButton.setImageResource(ThemeUtils.isNightTheme() ? R.drawable.ic_traffic_outdated_night
                                                       : R.drawable.ic_traffic_outdated);
  }

  public void setLoading(boolean isLoading)
  {
    if (isLoading)
    {
      mLoadingAnim.start();
      mButton.setImageDrawable(mLoadingAnim);
    }
    else
    {
      mLoadingAnim.stop();
      mButton.setImageDrawable(null);
    }
    mButton.invalidate();
  }

  public void setTopOffset(int offset)
  {
    RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) mButton.getLayoutParams();
    params.setMargins(0, offset, 0, 0);
    mButton.requestLayout();
  }

  public void show()
  {
    Animations.appearSliding(mButton, Animations.LEFT, null);
  }

  public void hide()
  {
    Animations.disappearSliding(mButton, Animations.LEFT, null);
  }
}
