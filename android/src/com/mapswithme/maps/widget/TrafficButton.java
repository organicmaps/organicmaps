package com.mapswithme.maps.widget;

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
    button.setLayoutParams(params);
    //TODO: set default value(state) here
    button.setVisibility(View.VISIBLE);
  }

  //TODO: handle the traffic state coming from the core here
  public void update(int state)
  {
    //TODO: soon
  }

  private void turnOff()
  {
    //TODO: soon
  }

  private void turnOn()
  {
   //TODO: soon
  }

  private void onTrafficOutdated()
  {
    //TODO: soon
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
}
