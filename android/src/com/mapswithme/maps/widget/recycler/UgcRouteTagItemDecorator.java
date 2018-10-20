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
    List<FlexLine> flexLines = flexboxLayoutManager.getFlexLines();
    if (flexLines == null || flexLines.isEmpty())
    {
      outRect.top = 0;
      return;
    }

    FlexLine flexLine = flexLines.get(0);
    int position = parent.getLayoutManager().getPosition(view);
    int itemCount = flexLine.getItemCount();
    if (position < itemCount)
      outRect.top = 0;
  }

  private boolean hasSpaceFromRight(Rect outRect, View view, RecyclerView parent)
  {
    int padding = parent.getPaddingLeft() + parent.getRight();
    return mCurrentOffset + view.getWidth() + outRect.left < parent.getWidth() - padding;
  }
}
