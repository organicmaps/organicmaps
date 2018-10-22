package com.mapswithme.maps.widget.recycler;

import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.View;

import com.google.android.flexbox.FlexLine;
import com.google.android.flexbox.FlexboxLayoutManager;

import java.util.List;

public class UgcRouteTagItemDecorator extends TagItemDecoration
{
  private int mCurrentOffset;

  public UgcRouteTagItemDecorator(@NonNull Drawable divider)
  {
    super(divider);
  }

  @Override
  public void getItemOffsets(Rect outRect, View view, RecyclerView parent, RecyclerView.State state)
  {
    super.getItemOffsets(outRect, view, parent, state);
    if (hasSpaceFromRight(outRect, view, parent))
      mCurrentOffset += view.getWidth() + outRect.left;
    else
      mCurrentOffset = 0;

    outRect.left = mCurrentOffset == 0 ? 0 : getDivider().getIntrinsicWidth() / 2;
    outRect.right = getDivider().getIntrinsicWidth() / 2;
    FlexboxLayoutManager flexboxLayoutManager = (FlexboxLayoutManager) parent.getLayoutManager();
    boolean isFirstLine = isFirstLineItem(view, parent, flexboxLayoutManager);
    if (isFirstLine)
      outRect.top = 0;
  }

  private static boolean isFirstLineItem(@NonNull View view, @NonNull RecyclerView parent,
                                         @NonNull FlexboxLayoutManager layoutManager)
  {
    List<FlexLine> flexLines = layoutManager.getFlexLines();
    if (flexLines == null || flexLines.isEmpty())
      return true;

    FlexLine flexLine = flexLines.iterator().next();
    int position = parent.getLayoutManager().getPosition(view);
    int itemCount = flexLine.getItemCount();
    return position < itemCount;
  }

  private boolean hasSpaceFromRight(@NonNull Rect outRect, @NonNull View view,
                                    @NonNull RecyclerView parent)
  {
    int padding = parent.getPaddingLeft() + parent.getRight();
    return mCurrentOffset + view.getWidth() + outRect.left < parent.getWidth() - padding;
  }
}
