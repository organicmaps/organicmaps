package com.mapswithme.maps.widget.recycler;

import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.View;

public class UgcRouteTagItemDecorator extends TagItemDecoration
{
  public UgcRouteTagItemDecorator(@NonNull Drawable divider)
  {
    super(divider);
  }

  private int mCurrentOffset;

  @Override
  public void getItemOffsets(Rect outRect, View view, RecyclerView parent, RecyclerView.State state)
  {
    super.getItemOffsets(outRect, view, parent, state);

    if (isLineFull(outRect, view, parent))
    {
      mCurrentOffset = 0;
    }
    else
    {
      mCurrentOffset += view.getWidth() + outRect.left;
    }

    if (mCurrentOffset == 0)
    {
      outRect.left = 0;
      outRect.right = getDivider().getIntrinsicWidth() / 2;
    }
    else
    {
      outRect.left = getDivider().getIntrinsicWidth() / 2;
      outRect.right = getDivider().getIntrinsicWidth() / 2;
    }
  }

  private boolean isLineFull(Rect outRect, View view, RecyclerView parent)
  {
    return mCurrentOffset + view.getWidth() + outRect.left > parent.getWidth() - (parent.getPaddingLeft() + parent.getRight());
  }
}
