package com.mapswithme.maps.widget.recycler;

import android.content.Context;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.support.annotation.NonNull;
import android.support.v7.widget.*;
import android.view.View;

class SponsoredDividerItemDecoration extends DividerItemDecoration
{
  private int mDividerWidth;
  /**
   * Creates a divider {@link RecyclerView.ItemDecoration} that can be used with a
   * {@link LinearLayoutManager}.
   *
   * @param context     Current context, it will be used to access resources.
   * @param orientation Divider orientation. Should be {@link #HORIZONTAL} or {@link #VERTICAL}.
   */
  SponsoredDividerItemDecoration(Context context, int orientation)
  {
    super(context, orientation);
  }

  @Override
  public void getItemOffsets(Rect outRect, View view, RecyclerView parent, RecyclerView.State state)
  {
    super.getItemOffsets(outRect, view, parent, state);
    // First element.
    if (parent.getChildAdapterPosition(view) == 0)
      outRect.left = mDividerWidth;
  }

  @Override
  public void setDrawable(@NonNull Drawable drawable)
  {
    super.setDrawable(drawable);
    mDividerWidth = drawable.getIntrinsicWidth();
  }
}
