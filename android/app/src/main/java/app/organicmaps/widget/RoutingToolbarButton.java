package app.organicmaps.widget;

import android.content.Context;
import android.util.AttributeSet;
import androidx.annotation.ColorRes;
import androidx.annotation.DrawableRes;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.appcompat.widget.AppCompatRadioButton;
import app.organicmaps.R;
import app.organicmaps.util.ThemeUtils;

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
    final boolean isNightTheme = ThemeUtils.isNightTheme();
    setBackgroundResource(isNightTheme ? R.drawable.routing_toolbar_button_night : R.drawable.routing_toolbar_button);
    setButtonTintList(isNightTheme ? R.color.routing_toolbar_icon_tint_night : R.color.routing_toolbar_icon_tint);
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
    setButtonTintList(AppCompatResources.getColorStateList(getContext(), color));
  }

  public void setIcon(@DrawableRes int icon)
  {
    mIcon = icon;
    setButtonDrawable(icon);
  }
}
