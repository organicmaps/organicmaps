package com.mapswithme.maps.widget.recycler;

import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.View;

/**
 * Adds interior dividers to a RecyclerView with a TagLayoutManager or its
 * subclass.
 */
public class TagItemDecoration extends RecyclerView.ItemDecoration
{
  @NonNull
  private final Drawable mDivider;

  public TagItemDecoration(@NonNull Drawable divider)
  {
    mDivider = divider;
  }

  /**
   * Draws horizontal and vertical dividers onto the parent RecyclerView.
   *
   * @param canvas The {@link Canvas} onto which dividers will be drawn
   * @param parent The RecyclerView onto which dividers are being added
   * @param state  The current RecyclerView.State of the RecyclerView
   */
  @Override
  public void onDraw(Canvas canvas, RecyclerView parent, RecyclerView.State state)
  {
    if (state.isMeasuring())
      return;

    int parentRight = parent.getWidth() - parent.getPaddingRight();
    int parentLeft = parent.getPaddingLeft();
    int lastHeight = Integer.MIN_VALUE;

    int childCount = parent.getChildCount();
    for (int i = 0; i < childCount; i++)
    {
      View child = parent.getChildAt(i);

      if (child.getTop() <= lastHeight)
      {
        mDivider.setBounds(child.getLeft() - mDivider.getIntrinsicWidth(),
                           child.getTop(),
                           child.getLeft(),
                           child.getBottom());
      }
      else
      {
        mDivider.setBounds(parentLeft,
                           child.getTop() - mDivider.getIntrinsicHeight(),
                           parentRight,
                           child.getTop());
      }
      mDivider.draw(canvas);
      lastHeight = child.getTop();
    }
  }

  /**
   * Determines the size and location of offsets between items in the parent
   * RecyclerView.
   *
   * @param outRect The {@link Rect} of offsets to be added around the child
   *                view
   * @param view    The child view to be decorated with an offset
   * @param parent  The RecyclerView onto which dividers are being added
   * @param state   The current RecyclerView.State of the RecyclerView
   */
  @Override
  public void getItemOffsets(Rect outRect, View view, RecyclerView parent, RecyclerView.State state)
  {
    super.getItemOffsets(outRect, view, parent, state);

    outRect.left = mDivider.getIntrinsicWidth();
    outRect.top = mDivider.getIntrinsicHeight();
  }

  @NonNull
  protected Drawable getDivider()
  {
    return mDivider;
  }
}
