package com.mapswithme.maps.widget;

import android.content.Context;
import android.os.Build;
import android.support.annotation.ColorRes;
import android.support.v7.widget.AppCompatRadioButton;
import android.util.AttributeSet;

import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;

public class RoutingToolbarButton extends AppCompatRadioButton
{
  private boolean mProgress;

  public RoutingToolbarButton(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    initView();
  }

  public RoutingToolbarButton(Context context, AttributeSet attrs)
  {
    super(context, attrs);
    initView();
  }

  public RoutingToolbarButton(Context context)
  {
    super(context);
    initView();
  }

  private void initView()
  {
    setBackgroundResource(ThemeUtils.isNightTheme() ? R.drawable.routing_toolbar_button_night
                                                    : R.drawable.routing_toolbar_button);
    setButtonTintList(ThemeUtils.isNightTheme() ? R.color.routing_toolbar_icon_tint_night
                                                : R.color.routing_toolbar_icon_tint);
  }

  public void progress()
  {
    if (mProgress)
      return;

    mProgress = true;
    setActivated(false);
    setSelected(true);
  }

  public void setProgress(boolean progress)
  {
    mProgress = progress;
  }

  public void error()
  {
    setActivated(true);
  }

  public void activate()
  {
    if (!mProgress)
    {
      setSelected(false);
      setActivated(true);
    }
  }

  public void deactivate()
  {
    setActivated(false);
    mProgress = false;
  }

  public void setButtonTintList(@ColorRes int color)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP)
      setSupportButtonTintList(getResources().getColorStateList(color));
    else
      setButtonTintList(getResources().getColorStateList(color));
  }
}
