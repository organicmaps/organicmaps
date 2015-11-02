package com.mapswithme.maps.widget;

import android.view.View;
import android.widget.AbsListView;

public class ListShadowController extends BaseShadowController<AbsListView>
{
  public ListShadowController(AbsListView list)
  {
    super(list);
  }

  @Override
  protected boolean shouldShowShadow(int id)
  {
    switch (id)
    {
    case TOP:
      int first = mList.getFirstVisiblePosition();
      if (first > 0)
        return true;

      View child = mList.getChildAt(0);
      return (child.getTop() < 0);

    case BOTTOM:
      int last = mList.getFirstVisiblePosition() + mList.getChildCount();
      if (last < mList.getAdapter().getCount())
        return true;

      child = mList.getChildAt(last - 1);
      return (child.getBottom() > mList.getHeight());

    default:
      throw new IllegalArgumentException("Invalid shadow id: " + id);
    }
  }

  @Override
  public BaseShadowController attach()
  {
    super.attach();

    mList.setOnScrollListener(new AbsListView.OnScrollListener()
    {
      @Override
      public void onScrollStateChanged(AbsListView view, int scrollState) {}

      @Override
      public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount, int totalItemCount)
      {
        updateShadows();
      }
    });


    updateShadows();
    return this;
  }

  @Override
  public void detach()
  {
    super.detach();
    mList.setOnScrollListener(null);
  }
}
