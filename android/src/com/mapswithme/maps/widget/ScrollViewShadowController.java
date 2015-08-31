package com.mapswithme.maps.widget;

import android.view.View;

public class ScrollViewShadowController extends BaseShadowController<ObservableScrollView>
{
  public ScrollViewShadowController(ObservableScrollView list)
  {
    super(list);
  }

  @Override
  protected boolean shouldShowShadow(int id)
  {
    if (mList.getChildCount() == 0)
      return false;

    switch (id)
    {
    case TOP:
      return (mList.getScrollY() > 0);

    case BOTTOM:
      View child = mList.getChildAt(0);
      return (mList.getScrollY() + mList.getHeight() < child.getHeight());

    default:
      throw new IllegalArgumentException("Invalid shadow id: " + id);
    }
  }

  @Override
  public BaseShadowController attach()
  {
    super.attach();
    mList.setScrollListener(new ObservableScrollView.SimpleScrollListener()
    {
      @Override
      public void onScroll(int left, int top)
      {
        updateShadows();
      }
    });

    return this;
  }

  @Override
  public void detach()
  {
    super.detach();
    mList.setScrollListener(null);
  }
}
