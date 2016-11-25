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
  private float mBottom;
  private float mTop;

  private boolean mIsZoomAnimate;
  private boolean mIsMyPosAnimate;
  private boolean mIsSlideDown;

  private final float mMenuHeight;

  NavigationButtonsAnimationController(@NonNull View zoomIn, @NonNull View zoomOut,
                                       @NonNull View myPosition)
  {
    mZoomIn = zoomIn;
    mZoomOut = zoomOut;
    UiUtils.showIf(showZoomButtons(), mZoomIn, mZoomOut);
    mMyPosition = myPosition;
    Resources res = mZoomIn.getResources();
    mMargin = res.getDimension(R.dimen.margin_base_plus);
    mMenuHeight = res.getDimension(R.dimen.menu_line_height);
    calculateLimitTranslations();
  }

  private void calculateLimitTranslations()
  {
    mTop = mMargin;
    mMyPosition.addOnLayoutChangeListener(new View.OnLayoutChangeListener()
    {
      @Override
      public void onLayoutChange(View v, int left, int top, int right, int bottom,
                                 int oldLeft, int oldTop, int oldRight, int oldBottom)
      {
        mBottom = bottom + mMargin;
        mMyPosition.removeOnLayoutChangeListener(this);
      }
    });
  }

  void setTopLimit(float limit)
  {
    mTop = limit + mMargin;
    update();
  }

  private void fadeOutZoom()
  {
    if (mIsSlideDown)
      return;
    mIsZoomAnimate = true;
    Animations.fadeOutView(mZoomIn, new Runnable()
    {
      @Override
      public void run()
      {
        mZoomIn.setVisibility(View.INVISIBLE);
        mIsZoomAnimate = false;
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
    mIsZoomAnimate = true;
    mZoomIn.setVisibility(View.VISIBLE);
    mZoomOut.setVisibility(View.VISIBLE);
    Animations.fadeInView(mZoomIn, new Runnable()
    {
      @Override
      public void run()
      {
        mIsZoomAnimate = false;
      }
    });
    Animations.fadeInView(mZoomOut, null);
  }

  private void fadeOutMyPosition()
  {
    if (mIsSlideDown)
      return;

    mIsMyPosAnimate = true;
    Animations.fadeOutView(mMyPosition, new Runnable()
    {
      @Override
      public void run()
      {
        UiUtils.invisible(mMyPosition);
        mIsMyPosAnimate = false;
      }
    });
  }

  private void fadeInMyPosition()
  {
    mIsMyPosAnimate = true;
    mMyPosition.setVisibility(View.VISIBLE);
    Animations.fadeInView(mMyPosition, new Runnable()
    {
      @Override
      public void run()
      {
        mIsMyPosAnimate = false;
      }
    });
  }

  void onPlacePageMoved(float translationY)
  {
    if (UiUtils.isLandscape(mMyPosition.getContext()) || mBottom == 0)
      return;

    final float amount = mZoomIn.getTranslationY() > 0 ? mMenuHeight : 0;
    final float translation = translationY - mBottom;
    update(translation <= amount ? translation : amount);
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
    if (!mIsZoomAnimate && isOverTopLimit(mZoomIn))
    {
      fadeOutZoom();
    }
    else if (!mIsZoomAnimate && satisfyTopLimit(mZoomIn))
    {
      fadeInZoom();
    }

    if (!shouldBeHidden() && !mIsMyPosAnimate
        && isOverTopLimit(mMyPosition))
    {
      fadeOutMyPosition();
    }
    else if (!shouldBeHidden() && !mIsMyPosAnimate
             && satisfyTopLimit(mMyPosition))
    {
      fadeInMyPosition();
    }
  }

  private boolean isOverTopLimit(@NonNull View view)
  {
    return view.getVisibility() == View.VISIBLE && view.getY() <= mTop;
  }

  private boolean satisfyTopLimit(@NonNull View view)
  {
    return view.getVisibility() == View.INVISIBLE && view.getY() >= mTop;
  }

  private boolean shouldBeHidden()
  {
    return LocationState.getMode() == LocationState.FOLLOW_AND_ROTATE
           && (RoutingController.get().isPlanning() || RoutingController.get().isNavigating());
  }

  void slide(boolean isDown)
  {
    if (UiUtils.isLandscape(mMyPosition.getContext())
        || (!isDown && mZoomIn.getTranslationY() <= 0))
      return;

    mIsSlideDown = isDown;

    ValueAnimator animator = ValueAnimator.ofFloat(isDown ? 0 : mMenuHeight, isDown ? mMenuHeight : 0);
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
        mIsSlideDown = false;
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
