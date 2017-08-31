package com.mapswithme.maps.widget.menu;

import android.animation.Animator;
import android.support.annotation.DimenRes;
import android.support.annotation.IdRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;

public abstract class BaseMenu
{
  public static final int ANIMATION_DURATION = MwmApplication.get().getResources().getInteger(R.integer.anim_menu);

  private boolean mIsOpen;
  private boolean mAnimating;

  final View mFrame;
  final View mLineFrame;
  final View mContentFrame;

  private final ItemClickListener mItemClickListener;

  int mContentHeight;

  boolean mLayoutMeasured;


  public interface Item
  {
    @IdRes int getViewId();
  }

  public interface ItemClickListener<T extends Item>
  {
    void onItemClick(T item);
  }

  private class AnimationListener extends UiUtils.SimpleAnimatorListener
  {
    @Override
    public void onAnimationStart(android.animation.Animator animation)
    {
      mAnimating = true;
    }

    @Override
    public void onAnimationEnd(android.animation.Animator animation)
    {
      mAnimating = false;
    }
  }

  View mapItem(final Item item, View frame)
  {
    View res = frame.findViewById(item.getViewId());
    if (res != null)
    {
      res.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          //noinspection unchecked
          mItemClickListener.onItemClick(item);
        }
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

  void measureContent(@Nullable final Runnable procAfterMeasurement)
  {
    if (mLayoutMeasured)
      return;

    UiUtils.measureView(mContentFrame, new UiUtils.OnViewMeasuredListener()
    {
      @Override
      public void onViewMeasured(int width, int height)
      {
        if (height != 0)
        {
          mContentHeight = height;
          mLayoutMeasured = true;

          UiUtils.hide(mContentFrame);
        }
        afterLayoutMeasured(procAfterMeasurement);
      }
    });
  }

  public void onResume(@Nullable Runnable procAfterMeasurement)
  {
    measureContent(procAfterMeasurement);
    updateMarker();
  }

  BaseMenu(@NonNull View frame, @NonNull ItemClickListener<? extends Item> itemClickListener)
  {
    mFrame = frame;
    mItemClickListener = itemClickListener;

    mLineFrame = mFrame.findViewById(R.id.line_frame);
    mContentFrame = mFrame.findViewById(R.id.content_frame);

    adjustTransparency();
  }

  public boolean isOpen()
  {
    return mIsOpen;
  }

  public boolean isAnimating()
  {
    return mAnimating;
  }

  public boolean open(boolean animate)
  {
    if ((animate && mAnimating) || isOpen())
      return false;

    mIsOpen = true;

    UiUtils.show(mContentFrame);
    adjustCollapsedItems();
    adjustTransparency();
    updateMarker();

    setToggleState(mIsOpen, animate);
    if (!animate)
      return true;

    mFrame.setTranslationY(mContentHeight);
    mFrame.animate()
          .setDuration(ANIMATION_DURATION)
          .translationY(0.0f)
          .setListener(new AnimationListener())
          .start();

    return true;
  }

  public boolean close(boolean animate)
  {
    return close(animate, null);
  }

  public boolean close(boolean animate, @Nullable final Runnable onCloseListener)
  {
    if (mAnimating || !isOpen())
    {
      if (onCloseListener != null)
        onCloseListener.run();

      return false;
    }

    mIsOpen = false;
    adjustCollapsedItems();
    setToggleState(mIsOpen, animate);

    if (!animate)
    {
      UiUtils.hide(mContentFrame);
      adjustTransparency();
      updateMarker();

      if (onCloseListener != null)
        onCloseListener.run();

      return true;
    }

    mFrame.animate()
          .setDuration(ANIMATION_DURATION)
          .translationY(mContentHeight)
          .setListener(new AnimationListener()
          {
            @Override
            public void onAnimationEnd(Animator animation)
            {
              super.onAnimationEnd(animation);
              mFrame.setTranslationY(0.0f);
              UiUtils.hide(mContentFrame);
              adjustTransparency();
              updateMarker();

              if (onCloseListener != null)
                onCloseListener.run();
            }
          }).start();

    return true;
  }

  public void toggle(boolean animate)
  {
    if (mAnimating)
      return;

    boolean show = !isOpen();

    if (show)
      open(animate);
    else
      close(animate);
  }

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

  protected void adjustCollapsedItems() {}
  protected void updateMarker() {}
  protected void setToggleState(boolean open, boolean animate) {}

  protected abstract @DimenRes int getHeightResId();
}
