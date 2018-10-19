package com.mapswithme.maps.widget.recycler;

import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.View;

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
  }

  private boolean hasSpaceFromRight(Rect outRect, View view, RecyclerView parent)
  {
    int padding = parent.getPaddingLeft() + parent.getRight();
    return mCurrentOffset + view.getWidth() + outRect.left < parent.getWidth() - padding;
  }
}
