package com.mapswithme.maps.widget.placepage;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;

import androidx.coordinatorlayout.widget.CoordinatorLayout;
import com.google.android.material.appbar.AppBarLayout;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

@SuppressWarnings("unused")
public class ToolbarBehavior extends AppBarLayout.ScrollingViewBehavior
{
  private boolean mBookmarkMode;

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
  public boolean onDependentViewChanged(CoordinatorLayout parent, View toolbar, View placePage)
  {
    if (placePage.getY() == 0 && UiUtils.isHidden(toolbar))
    {
      UiUtils.show(toolbar);
      return false;
    }

    if (placePage.getY() > 0 && UiUtils.isVisible(toolbar))
    {
      UiUtils.hide(toolbar);
      return false;
    }

    return false;
  }
}
