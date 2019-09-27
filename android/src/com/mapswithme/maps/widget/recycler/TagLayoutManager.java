package com.mapswithme.maps.widget.recycler;

import androidx.recyclerview.widget.RecyclerView;
import android.view.View;

/**
 * This LayoutManager designed only for use with RecyclerView.setNestedScrollingEnabled(false)
 * and recycle item must be wrap_content or fixed size
 */
public class TagLayoutManager extends RecyclerView.LayoutManager
{
  public TagLayoutManager()
  {
    setAutoMeasureEnabled(true);
  }

  @Override
  public RecyclerView.LayoutParams generateDefaultLayoutParams()
  {
    return new RecyclerView.LayoutParams(RecyclerView.LayoutParams.MATCH_PARENT,
                                         RecyclerView.LayoutParams.WRAP_CONTENT);
  }

  @Override
  public void onLayoutChildren(RecyclerView.Recycler recycler, RecyclerView.State state)
  {
    detachAndScrapAttachedViews(recycler);

    int widthUsed = 0;
    int heightUsed = 0;
    int lineHeight = 0;
    int itemsCountOneLine = 0;
    for (int i = 0; i < getItemCount(); i++)
    {
      View child = recycler.getViewForPosition(i);
      addView(child);
      measureChildWithMargins(child, widthUsed, heightUsed);
      int width = getDecoratedMeasuredWidth(child);
      int height = getDecoratedMeasuredHeight(child);
      lineHeight = Math.max(lineHeight, height);
      if (widthUsed + width >= getWidth())
      {
        widthUsed = 0;
        if (itemsCountOneLine > 0)
        {
          itemsCountOneLine = -1;
          heightUsed += lineHeight;
          child.forceLayout();
          measureChildWithMargins(child, widthUsed, heightUsed);
          width = getDecoratedMeasuredWidth(child);
          height = getDecoratedMeasuredHeight(child);
        }
        lineHeight = 0;
      }
      layoutDecorated(child, widthUsed, heightUsed, widthUsed + width, heightUsed + height);
      widthUsed += width;
      itemsCountOneLine++;
    }
  }
}
