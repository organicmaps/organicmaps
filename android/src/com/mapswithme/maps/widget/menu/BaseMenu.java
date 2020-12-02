package com.mapswithme.maps.widget.menu;

import android.view.View;

import androidx.annotation.DimenRes;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;

public abstract class BaseMenu
{
  final View mFrame;
  final View mLineFrame;

  private final ItemClickListener mItemClickListener;

  public interface Item
  {
    @IdRes int getViewId();
  }

  public interface ItemClickListener<T extends Item>
  {
    void onItemClick(T item);
  }

  View mapItem(final Item item, View frame)
  {
    View res = frame.findViewById(item.getViewId());
    if (res != null)
    {
      res.setOnClickListener(v -> {
        //noinspection unchecked
        mItemClickListener.onItemClick(item);
      });
    }
    return res;
  }

  protected void adjustTransparency()
  {
    mFrame.setBackgroundColor(ThemeUtils.getColor(mFrame.getContext(), isOpen() ? R.attr.menuBackgroundOpen
                                                                                : R.attr.menuBackground));
  }

  void afterLayoutMeasured(@Nullable Runnable procAfterCorrection)
  {
    if (procAfterCorrection != null)
      procAfterCorrection.run();
  }

  public abstract void onResume(@Nullable Runnable procAfterMeasurement);

  BaseMenu(@NonNull View frame, @NonNull ItemClickListener<? extends Item> itemClickListener)
  {
    mFrame = frame;
    mItemClickListener = itemClickListener;

    mLineFrame = mFrame.findViewById(R.id.line_frame);

    adjustTransparency();
  }

  public abstract boolean isOpen();

  public abstract boolean isAnimating();

  public abstract boolean open(boolean animate);

  public boolean close(boolean animate)
  {
    return close(animate, null);
  }

  public abstract boolean close(boolean animate, @Nullable final Runnable onCloseListener);

  public abstract void toggle(boolean animate);

  public void show(boolean show)
  {
    if (show && mFrame.isShown())
      return;

    UiUtils.showIf(show, mFrame);
  }

  public View getFrame()
  {
    return mFrame;
  }

  public void updateMarker() {}
  protected void setToggleState(boolean open, boolean animate) {}

  protected abstract @DimenRes int getHeightResId();
}
