package com.mapswithme.maps.widget.recycler;

import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.support.annotation.Dimension;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.View;

public class DotDividerItemDecoration extends RecyclerView.ItemDecoration
{
  @Dimension
  private final int mHorizontalMargin;
  @Dimension
  private final int mVerticalMargin;
  @NonNull
  private final Drawable mDivider;

  public DotDividerItemDecoration(@NonNull Drawable divider, @Dimension int horizontalMargin,
                                  @Dimension int verticalMargin)
  {
    mDivider = divider;
    mHorizontalMargin = horizontalMargin;
    mVerticalMargin = verticalMargin;
  }

  @Override
  public void getItemOffsets(Rect outRect, View view, RecyclerView parent, RecyclerView.State state)
  {
    super.getItemOffsets(outRect, view, parent, state);
    outRect.right = mHorizontalMargin;
    outRect.bottom = mVerticalMargin;
  }

  @Override
  public void onDraw(Canvas c, RecyclerView parent, RecyclerView.State state)
  {
    if (state.isMeasuring())
      return;

    int childCount = parent.getChildCount();
    for (int i = 0; i < childCount - 1; i++)
    {
      View child = parent.getChildAt(i);

      int centerX = mHorizontalMargin / 2 + child.getRight();
      int centerY = child.getHeight() / 2 + child.getTop();
      int left = centerX - mDivider.getIntrinsicWidth() / 2;
      int right = left + mDivider.getIntrinsicWidth();
      int top = centerY - mDivider.getIntrinsicHeight() / 2;
      int bottom = top + mDivider.getIntrinsicHeight();

      mDivider.setBounds(left, top, right, bottom);

      mDivider.draw(c);
    }
  }
}
