package com.mapswithme.maps.adapter;

import android.content.Context;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.AttributeSet;
import android.view.ViewGroup;

public class SpanningLinearLayoutManager extends LinearLayoutManager
{
  public SpanningLinearLayoutManager(Context context)
  {
    super(context);
  }

  public SpanningLinearLayoutManager(Context context, int orientation, boolean reverseLayout)
  {
    super(context, orientation, reverseLayout);
  }

  public SpanningLinearLayoutManager(Context context, AttributeSet attrs, int defStyleAttr, int
      defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
  }

  @Override
  public RecyclerView.LayoutParams generateDefaultLayoutParams()
  {
    return spanLayoutSize(super.generateDefaultLayoutParams());
  }

  @Override
  public RecyclerView.LayoutParams generateLayoutParams(Context c, AttributeSet attrs)
  {
    return spanLayoutSize(super.generateLayoutParams(c, attrs));
  }

  @Override
  public RecyclerView.LayoutParams generateLayoutParams(ViewGroup.LayoutParams lp)
  {
    return spanLayoutSize(super.generateLayoutParams(lp));
  }

  @Override
  public boolean checkLayoutParams(RecyclerView.LayoutParams lp)
  {
    return super.checkLayoutParams(lp);
  }

  private RecyclerView.LayoutParams spanLayoutSize(RecyclerView.LayoutParams layoutParams)
  {
    if (getOrientation() == HORIZONTAL)
    {
      layoutParams.width = (int) Math.round(getHorizontalSpace() / (double) getItemCount());
    }
    else if (getOrientation() == VERTICAL)
    {
      layoutParams.height = (int) Math.round(getVerticalSpace() / (double) getItemCount());
    }
    return layoutParams;
  }

  @Override
  public boolean canScrollVertically()
  {
    return false;
  }

  @Override
  public boolean canScrollHorizontally()
  {
    return false;
  }

  private int getHorizontalSpace()
  {
    return getWidth() - getPaddingRight() - getPaddingLeft();
  }

  private int getVerticalSpace()
  {
    return getHeight() - getPaddingBottom() - getPaddingTop();
  }
}
