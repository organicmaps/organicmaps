package app.organicmaps.widget.recycler;

import android.view.View;
import androidx.annotation.Dimension;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

public class MultilineLayoutManager extends RecyclerView.LayoutManager
{
  private boolean reverseLayout = false;

  public MultilineLayoutManager(int layoutDirection)
  {
    setAutoMeasureEnabled(true);
    reverseLayout = layoutDirection == View.LAYOUT_DIRECTION_RTL;
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

    assertInLayoutOrScroll(null);

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
      if (width > getWidth() - widthUsed)
        width = squeezeChildIntoLine(widthUsed, heightUsed, child);
      int height = getDecoratedMeasuredHeight(child);
      lineHeight = Math.max(lineHeight, height);
      if (widthUsed + width > getWidth())
      {
        widthUsed = 0;
        if (itemsCountOneLine > 0)
        {
          itemsCountOneLine = 0;
          heightUsed += lineHeight;
          child.forceLayout();
          measureChildWithMargins(child, widthUsed, heightUsed);
          width = getDecoratedMeasuredWidth(child);
          if (width > getWidth() - widthUsed)
            width = squeezeChildIntoLine(widthUsed, heightUsed, child);
          height = getDecoratedMeasuredHeight(child);
        }
        lineHeight = 0;
      }
      if (reverseLayout)
        layoutDecoratedWithMargins(child, getWidth() - widthUsed - width, heightUsed, getWidth() - widthUsed,
                                   heightUsed + height);
      else
        layoutDecorated(child, widthUsed, heightUsed, widthUsed + width, heightUsed + height);
      widthUsed += width;
      itemsCountOneLine++;
    }
  }

  private int squeezeChildIntoLine(int widthUsed, int heightUsed, @NonNull View child)
  {
    if (!(child instanceof SqueezingInterface))
      return getDecoratedMeasuredWidth(child);

    int availableWidth = getWidth() - widthUsed - getDecoratedRight(child);
    if (availableWidth > ((SqueezingInterface) child).getMinimumAcceptableSize())
    {
      ((SqueezingInterface) child).squeezeTo(availableWidth);
      child.forceLayout();
      measureChildWithMargins(child, widthUsed, heightUsed);
    }
    return getDecoratedMeasuredWidth(child);
  }

  public interface SqueezingInterface
  {
    void squeezeTo(@Dimension int width);

    @Dimension
    int getMinimumAcceptableSize();
  }
}
