package com.mapswithme.maps;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.content.res.Resources;
import android.support.annotation.NonNull;
import android.view.View;

import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.Animations;
import com.mapswithme.util.Config;
import com.mapswithme.util.UiUtils;

class NavigationButtonsAnimationController
{
  private static final int ANIM_TOGGLE = MwmApplication.get().getResources().getInteger(R.integer.anim_slots_toggle);

  @NonNull
  private final View mZoomIn;
  @NonNull
  private final View mZoomOut;
  @NonNull
  private final View mMyPosition;

  private final float mMargin;
  private float mContentHeight;
  private float mMyPositionBottom;

  private boolean mZoomAnimate;
  private boolean mMyPosAnimate;
  private boolean mSlideDown;

  private float mTopLimit;
  private float mBottomLimit;

  private float mCurrentOffset;

  NavigationButtonsAnimationController(@NonNull View zoomIn, @NonNull View zoomOut,
                                       @NonNull View myPosition, @NonNull final View contentView)
  {
    mZoomIn = zoomIn;
    mZoomOut = zoomOut;
    UiUtils.showIf(showZoomButtons(), mZoomIn, mZoomOut);
    mMyPosition = myPosition;
    Resources res = mZoomIn.getResources();
    mMargin = res.getDimension(R.dimen.margin_base_plus);
    mBottomLimit = res.getDimension(R.dimen.menu_line_height);
    calculateLimitTranslations();
    contentView.addOnLayoutChangeListener(new View.OnLayoutChangeListener()
    {
      @Override
      public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft,
                                 int oldTop, int oldRight, int oldBottom)
      {
        mContentHeight = bottom - top;
        contentView.removeOnLayoutChangeListener(this);
      }
    });
  }

  private void calculateLimitTranslations()
  {
    mTopLimit = mMargin;
    mMyPosition.addOnLayoutChangeListener(new View.OnLayoutChangeListener()
    {
      @Override
      public void onLayoutChange(View v, int left, int top, int right, int bottom,
                                 int oldLeft, int oldTop, int oldRight, int oldBottom)
      {
        mMyPositionBottom = bottom;
        mMyPosition.removeOnLayoutChangeListener(this);
      }
    });
  }

  void setTopLimit(float limit)
  {
    mTopLimit = limit + mMargin;
    update();
  }

  void setBottomLimit(float limit)
  {
    mBottomLimit = limit;
    update();
  }

  private void fadeOutZoom()
  {
    if (mSlideDown)
      return;

    mZoomAnimate = true;
    Animations.fadeOutView(mZoomIn, new Runnable()
    {
      @Override
      public void run()
      {
        mZoomIn.setVisibility(View.INVISIBLE);
        mZoomAnimate = false;
      }
    });
    Animations.fadeOutView(mZoomOut, new Runnable()
    {
      @Override
      public void run()
      {
        mZoomOut.setVisibility(View.INVISIBLE);
      }
    });
  }

  private void fadeInZoom()
  {
    mZoomAnimate = true;
    mZoomIn.setVisibility(View.VISIBLE);
    mZoomOut.setVisibility(View.VISIBLE);
    Animations.fadeInView(mZoomIn, new Runnable()
    {
      @Override
      public void run()
      {
        mZoomAnimate = false;
      }
    });
    Animations.fadeInView(mZoomOut, null);
  }

  private void fadeOutMyPosition()
  {
    if (mSlideDown)
      return;

    mMyPosAnimate = true;
    Animations.fadeOutView(mMyPosition, new Runnable()
    {
      @Override
      public void run()
      {
        UiUtils.invisible(mMyPosition);
        mMyPosAnimate = false;
      }
    });
  }

  private void fadeInMyPosition()
  {
    mMyPosAnimate = true;
    mMyPosition.setVisibility(View.VISIBLE);
    Animations.fadeInView(mMyPosition, new Runnable()
    {
      @Override
      public void run()
      {
        mMyPosAnimate = false;
      }
    });
  }

  void onPlacePageMoved(float translationY)
  {
    if (UiUtils.isLandscape(mMyPosition.getContext()) || mMyPositionBottom == 0 || mContentHeight == 0)
      return;

    final float amount = mCurrentOffset > 0 ? mMargin : -mMargin;
    final float translation = translationY - (mMyPositionBottom - mCurrentOffset + amount);
    update(translation <= mCurrentOffset ? translation : mCurrentOffset);
  }

  private void update()
  {
    update(mZoomIn.getTranslationY());
  }

  private void update(final float translation)
  {
    mMyPosition.setTranslationY(translation);
    mZoomOut.setTranslationY(translation);
    mZoomIn.setTranslationY(translation);
    if (!mZoomAnimate && mZoomIn.getVisibility() == View.VISIBLE
        && !isViewInsideLimits(mZoomIn))
    {
      fadeOutZoom();
    }
    else if (!mZoomAnimate && mZoomIn.getVisibility() == View.INVISIBLE
             && isViewInsideLimits(mZoomIn))
    {
      fadeInZoom();
    }

    if (!shouldBeHidden() && !mMyPosAnimate
        && mMyPosition.getVisibility() == View.VISIBLE && !isViewInsideLimits(mMyPosition))
    {
      fadeOutMyPosition();
    }
    else if (!shouldBeHidden() && !mMyPosAnimate
             && mMyPosition.getVisibility() == View.INVISIBLE && isViewInsideLimits(mMyPosition))
    {
      fadeInMyPosition();
    }
  }

  private boolean isViewInsideLimits(@NonNull View view)
  {
    return view.getY() >= mTopLimit &&
           view.getBottom() + view.getTranslationY() <= mContentHeight - mBottomLimit;
  }

  private boolean shouldBeHidden()
  {
    return LocationState.getMode() == LocationState.FOLLOW_AND_ROTATE
           && (RoutingController.get().isPlanning() || RoutingController.get().isNavigating());
  }

  void slide(boolean isDown, float distance)
  {
    if (UiUtils.isLandscape(mMyPosition.getContext())
        || (!isDown && mZoomIn.getTranslationY() <= 0))
      return;

    mSlideDown = isDown;
    mCurrentOffset = isDown ? distance : 0;

    ValueAnimator animator = ValueAnimator.ofFloat(isDown ? 0 : distance, isDown ? distance : 0);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        float value = (float) animation.getAnimatedValue();
        update(value);
      }
    });
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        mSlideDown = false;
        update();
      }
    });
    animator.setDuration(ANIM_TOGGLE);
    animator.start();
  }

  void disappearZoomButtons()
  {
    if (!showZoomButtons())
      return;

    Animations.disappearSliding(mZoomIn, Animations.RIGHT, null);
    Animations.disappearSliding(mZoomOut, Animations.RIGHT, null);
  }

  void appearZoomButtons()
  {
    if (!showZoomButtons())
      return;

    Animations.appearSliding(mZoomIn, Animations.LEFT, null);
    Animations.appearSliding(mZoomOut, Animations.LEFT, null);
  }

  private static boolean showZoomButtons()
  {
    return Config.showZoomButtons();
  }

  public void onResume()
  {
    update();
  }
}
