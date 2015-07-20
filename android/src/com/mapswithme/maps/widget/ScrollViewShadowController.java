package com.mapswithme.maps.widget;

import android.view.View;

public class ScrollViewShadowController extends BaseShadowController<ObservableScrollView,
                                                                     ObservableScrollView.ScrollListener>
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
    }

    return false;
  }

  @Override
  public BaseShadowController attach()
  {
    super.attach();
    mList.setScrollListener(new ObservableScrollView.ScrollListener() {
      @Override
      public void onScroll(int left, int top)
      {
        updateShadows();
        if (mScrollListener != null)
          mScrollListener.onScroll(left, top);
      }

      @Override
      public void onScrollEnd()
      {
        if (mScrollListener != null)
          mScrollListener.onScrollEnd();
      }
    });

    return this;
  }

  @Override
  public void detach()
  {
    super.detach();
    mList.setScrollListener(mScrollListener);
  }
}
