package com.mapswithme.maps.widget;

import android.content.Context;
import android.os.Build;
import androidx.annotation.ColorRes;
import androidx.annotation.DrawableRes;
import androidx.appcompat.widget.AppCompatRadioButton;
import android.util.AttributeSet;

import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;

public class RoutingToolbarButton extends AppCompatRadioButton
{
  private boolean mInProgress;
  @DrawableRes
  private int mIcon;

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
    setBackgroundResource(ThemeUtils.isNightTheme(getContext()) ? R.drawable.routing_toolbar_button_night
                                                                : R.drawable.routing_toolbar_button);
    setButtonTintList(ThemeUtils.isNightTheme(getContext()) ? R.color.routing_toolbar_icon_tint_night
                                                            : R.color.routing_toolbar_icon_tint);
  }

  public void progress()
  {
    if (mInProgress)
      return;

    setButtonDrawable(mIcon);
    mInProgress = true;
    setActivated(false);
    setSelected(true);
  }

  public void error()
  {
    mInProgress = false;
    setSelected(false);
    setButtonDrawable(R.drawable.ic_routing_error);
    setActivated(true);
  }

  public void activate()
  {
    if (!mInProgress)
    {
      setButtonDrawable(mIcon);
      setSelected(false);
      setActivated(true);
    }
  }

  public void complete()
  {
    mInProgress = false;
    activate();
  }

  public void deactivate()
  {
    setActivated(false);
    mInProgress = false;
  }

  public void setButtonTintList(@ColorRes int color)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP)
      setSupportButtonTintList(getResources().getColorStateList(color));
    else
      setButtonTintList(getResources().getColorStateList(color));
  }

  public void setIcon(@DrawableRes int icon)
  {
    mIcon = icon;
    setButtonDrawable(icon);
  }
}
