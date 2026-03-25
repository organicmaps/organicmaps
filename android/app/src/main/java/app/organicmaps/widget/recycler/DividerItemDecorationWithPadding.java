package app.organicmaps.widget.recycler;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.view.View;
import androidx.annotation.Dimension;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import java.util.Objects;

public class DividerItemDecorationWithPadding extends RecyclerView.ItemDecoration
{
  @Dimension
  private final int mStartMargin;
  @NonNull
  private final Drawable mDivider;

  public DividerItemDecorationWithPadding(@NonNull Context context)
  {
    mDivider = Objects.requireNonNull(AppCompatResources.getDrawable(context, R.drawable.divider_base));
    mStartMargin = context.getResources().getDimensionPixelSize(R.dimen.margin_quadruple_plus_half);
  }

  private boolean useFullWidth(@Nullable RecyclerView.ViewHolder holder)
  {
    return holder instanceof DividerBehavior && ((DividerBehavior) holder).useFullWidthDivider();
  }

  private boolean skipDivider(@NonNull RecyclerView.ViewHolder holder)
  {
    return holder instanceof DividerBehavior && ((DividerBehavior) holder).skipDivider();
  }

  @Override
  public void onDrawOver(@NonNull Canvas c, @NonNull RecyclerView parent, @NonNull RecyclerView.State state)
  {
    if (state.isMeasuring())
      return;

    final int right = parent.getWidth();
    final int dividerHeight = mDivider.getIntrinsicHeight();

    final int childCount = parent.getChildCount();
    for (int i = 0; i < childCount; i++)
    {
      View child = parent.getChildAt(i);
      View nextChild = parent.getChildAt(i + 1);
      RecyclerView.ViewHolder viewHolder = parent.getChildViewHolder(child);
      RecyclerView.ViewHolder viewHolderNext = null;
      if (nextChild != null)
        viewHolderNext = parent.getChildViewHolder(nextChild);

      int top = child.getBottom();
      int bottom = top + dividerHeight;

      // Full-width has priority
      if (useFullWidth(viewHolder) || useFullWidth(viewHolderNext))
      {
        mDivider.setBounds(0, top, right, bottom);
        mDivider.draw(c);
        continue;
      }

      // Skip if current item wants to skip
      if (skipDivider(viewHolder))
        continue;

      // Default: divider with margin (or full-width for last item)
      if (i == childCount - 1)
        mDivider.setBounds(0, top - dividerHeight, right, bottom);
      else
        mDivider.setBounds(mStartMargin, top, right, bottom);

      mDivider.draw(c);
    }
  }
}
