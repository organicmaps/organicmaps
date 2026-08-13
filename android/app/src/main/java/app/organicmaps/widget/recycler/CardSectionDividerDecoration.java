package app.organicmaps.widget.recycler;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import java.util.Objects;

/**
 * Draws a hairline between the rows of a card-grouped list: inset symmetrically so that it stays inside the card,
 * and only below rows whose holder does not {@link DividerBehavior#skipDivider()} — which is how the last row of a
 * section keeps its rounded bottom corners clean.
 */
public class CardSectionDividerDecoration extends RecyclerView.ItemDecoration
{
  @NonNull
  private final Drawable mDivider;
  private final int mInset;

  public CardSectionDividerDecoration(@NonNull Context context)
  {
    mDivider = Objects.requireNonNull(AppCompatResources.getDrawable(context, R.drawable.divider_base));
    mInset = context.getResources().getDimensionPixelSize(R.dimen.margin_base);
  }

  @Override
  public void onDrawOver(@NonNull Canvas c, @NonNull RecyclerView parent, @NonNull RecyclerView.State state)
  {
    if (state.isMeasuring())
      return;

    final int left = parent.getPaddingLeft() + mInset;
    final int right = parent.getWidth() - parent.getPaddingRight() - mInset;
    final int dividerHeight = mDivider.getIntrinsicHeight();

    final int childCount = parent.getChildCount();
    for (int i = 0; i < childCount; i++)
    {
      final View child = parent.getChildAt(i);
      // Rows can be collapsed to a zero-height stub while staying attached (see UiUtils.showRecyclerItemView);
      // drawing under them would stack hairlines at a single y.
      if (child.getVisibility() != View.VISIBLE || child.getHeight() == 0)
        continue;

      final RecyclerView.ViewHolder holder = parent.getChildViewHolder(child);
      if (!(holder instanceof DividerBehavior) || ((DividerBehavior) holder).skipDivider())
        continue;

      final int top = child.getBottom();
      mDivider.setBounds(left, top, right, top + dividerHeight);
      mDivider.draw(c);
    }
  }
}
