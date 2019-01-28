package com.mapswithme.maps.widget.placepage;

import android.content.Context;
import android.support.design.widget.AppBarLayout;
import android.support.design.widget.CoordinatorLayout;
import android.util.AttributeSet;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

@SuppressWarnings("unused")
public class ToolbarBehavior extends AppBarLayout.ScrollingViewBehavior
{
  public ToolbarBehavior()
  {
    // Do nothing by default.
  }

  public ToolbarBehavior(Context context, AttributeSet attrs)
  {
    super(context, attrs);
  }

  @Override
  public boolean layoutDependsOn(CoordinatorLayout parent, View child, View dependency)
  {
    return dependency.getId() == R.id.placepage;
  }

  @Override
  public boolean onDependentViewChanged(CoordinatorLayout parent, View child, View dependency)
  {
    if (dependency.getY() == 0 && UiUtils.isHidden(child))
    {
      UiUtils.show(child);
      return false;
    }

    if (dependency.getY() > 0 && UiUtils.isVisible(child))
    {
      UiUtils.hide(child);
      return false;
    }

    return false;
  }
}
