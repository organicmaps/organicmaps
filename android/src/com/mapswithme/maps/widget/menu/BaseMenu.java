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

  final View mFrame;
  final View mLineFrame;
  final View mContentFrame;

  private final ItemClickListener mItemClickListener;

  int mContentHeight;

  boolean mLayoutCorrected;
  boolean mAnimating;


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

  void afterLayoutCorrected(@Nullable Runnable procAfterCorrection)
  {
    if (procAfterCorrection != null)
      procAfterCorrection.run();
  }

  private void correctLayout(@Nullable final Runnable procAfterCorrection)
  {
    if (mLayoutCorrected)
      return;

    UiUtils.measureView(mContentFrame, new UiUtils.OnViewMeasuredListener()
    {
      @Override
      public void onViewMeasured(int width, int height)
      {
        mContentHeight = height;
        mLayoutCorrected = true;

        UiUtils.hide(mContentFrame);
        afterLayoutCorrected(procAfterCorrection);
      }
    });
  }

  public void onResume(@Nullable Runnable procAfterCorrection)
  {
    correctLayout(procAfterCorrection);
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
    return UiUtils.isVisible(mContentFrame);
  }

  public boolean isAnimating()
  {
    return mAnimating;
  }

  public boolean open(boolean animate)
  {
    if ((animate && mAnimating) || isOpen())
      return false;

    UiUtils.show(mContentFrame);
    adjustCollapsedItems();
    adjustTransparency();
    updateMarker();

    setToggleState(true, animate);
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

    adjustCollapsedItems();

    if (!animate)
    {
      UiUtils.hide(mContentFrame);
      adjustTransparency();
      updateMarker();

      setToggleState(false, false);

      if (onCloseListener != null)
        onCloseListener.run();

      return true;
    }

    setToggleState(false, true);

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
    close(false);
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