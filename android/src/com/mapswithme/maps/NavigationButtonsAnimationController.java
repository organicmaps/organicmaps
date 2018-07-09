package com.mapswithme.maps;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.content.res.Resources;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationState;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.Animations;
import com.mapswithme.util.Config;
import com.mapswithme.util.UiUtils;

class NavigationButtonsAnimationController
{
  private static final int ANIM_TOGGLE = MwmApplication.get().getResources().getInteger(R.integer.anim_default);
  private static final String STATE_VISIBLE = "state_visible";

  @NonNull
  private final View mZoomIn;
  @NonNull
  private final View mZoomOut;
  @NonNull
  private final View mMyPosition;

  @Nullable
  private final OnTranslationChangedListener mTranslationListener;

  private final float mMargin;
  private float mContentHeight;
  private float mMyPositionBottom;

  private boolean mZoomAnimating;
  private boolean mMyPosAnimating;
  private boolean mSlidingDown;
  private boolean mZoomVisible;

  private float mTopLimit;
  private float mBottomLimit;

  private float mCurrentOffset;
  private float mCompassHeight;

  NavigationButtonsAnimationController(@NonNull View zoomIn, @NonNull View zoomOut,
                                       @NonNull View myPosition, @NonNull final View contentView,
                                       @Nullable OnTranslationChangedListener translationListener)
  {
    mZoomIn = zoomIn;
    mZoomOut = zoomOut;
    mZoomVisible = UiUtils.isVisible(mZoomIn) && UiUtils.isVisible(mZoomOut);
    checkZoomButtonsVisibility();
    mMyPosition = myPosition;
    Resources res = mZoomIn.getResources();
    mMargin = res.getDimension(R.dimen.margin_base_plus);
    mBottomLimit = res.getDimension(R.dimen.menu_line_height);
    mCompassHeight = res.getDimension(R.dimen.compass_height);
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
    mTranslationListener = translationListener;
  }

  private void checkZoomButtonsVisibility()
  {
    UiUtils.showIf(showZoomButtons() && mZoomVisible, mZoomIn, mZoomOut);
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
    if (mSlidingDown)
      return;

    mZoomAnimating = true;
    Animations.fadeOutView(mZoomIn, new Runnable()
    {
      @Override
      public void run()
      {
        mZoomIn.setVisibility(View.INVISIBLE);
        mZoomAnimating = false;
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
    if (mTranslationListener != null)
      mTranslationListener.onFadeOutZoomButtons();
  }

  private void fadeInZoom()
  {
    mZoomAnimating = true;
    mZoomIn.setVisibility(View.VISIBLE);
    mZoomOut.setVisibility(View.VISIBLE);
    Animations.fadeInView(mZoomIn, new Runnable()
    {
      @Override
      public void run()
      {
        mZoomAnimating = false;
      }
    });
    Animations.fadeInView(mZoomOut, null);
    if (mTranslationListener != null)
      mTranslationListener.onFadeInZoomButtons();
  }

  private void fadeOutMyPosition()
  {
    if (mSlidingDown)
      return;

    mMyPosAnimating = true;
    Animations.fadeOutView(mMyPosition, new Runnable()
    {
      @Override
      public void run()
      {
        UiUtils.invisible(mMyPosition);
        mMyPosAnimating = false;
      }
    });
  }

  private void fadeInMyPosition()
  {
    mMyPosAnimating = true;
    mMyPosition.setVisibility(View.VISIBLE);
    Animations.fadeInView(mMyPosition, new Runnable()
    {
      @Override
      public void run()
      {
        mMyPosAnimating = false;
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
    if (!mZoomAnimating && mZoomIn.getVisibility() == View.VISIBLE
        && !isViewInsideLimits(mZoomIn))
    {
      fadeOutZoom();
    }
    else if (!mZoomAnimating && mZoomIn.getVisibility() == View.INVISIBLE
             && isViewInsideLimits(mZoomIn))
    {
      fadeInZoom();
    }

    if (!shouldBeHidden() && !mMyPosAnimating
        && mMyPosition.getVisibility() == View.VISIBLE && !isViewInsideLimits(mMyPosition))
    {
      fadeOutMyPosition();
    }
    else if (!shouldBeHidden() && !mMyPosAnimating
             && mMyPosition.getVisibility() == View.INVISIBLE && isViewInsideLimits(mMyPosition))
    {
      fadeInMyPosition();
    }
    if (mTranslationListener != null)
      mTranslationListener.onTranslationChanged(translation);
  }

  private boolean isViewInsideLimits(@NonNull View view)
  {
    return view.getY() >= mTopLimit &&
           view.getBottom() + view.getTranslationY() <= mContentHeight - mBottomLimit;
  }

  private boolean shouldBeHidden()
  {
    return LocationHelper.INSTANCE.getMyPositionMode() == LocationState.FOLLOW_AND_ROTATE
           && (RoutingController.get().isPlanning() || RoutingController.get().isNavigating());
  }

  void slide(boolean isDown, float distance)
  {
    if (UiUtils.isLandscape(mMyPosition.getContext())
        || (!isDown && mZoomIn.getTranslationY() <= 0))
      return;

    mSlidingDown = isDown;
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
        mSlidingDown = false;
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

    mZoomVisible = false;
    Animations.disappearSliding(mZoomIn, Animations.RIGHT, null);
    Animations.disappearSliding(mZoomOut, Animations.RIGHT, null);
  }

  void appearZoomButtons()
  {
    if (!showZoomButtons())
      return;

    mZoomVisible = true;
    Animations.appearSliding(mZoomIn, Animations.LEFT, null);
    Animations.appearSliding(mZoomOut, Animations.LEFT, null);
  }

  private static boolean showZoomButtons()
  {
    return Config.showZoomButtons();
  }

  public void onResume()
  {
    checkZoomButtonsVisibility();
  }

  public void onSaveState(@NonNull Bundle outState)
  {
    outState.putBoolean(STATE_VISIBLE, mZoomVisible);
  }

  public void onRestoreState(@NonNull Bundle state)
  {
    mZoomVisible = state.getBoolean(STATE_VISIBLE, false);
  }

  boolean isConflictWithCompass(int compassOffset)
  {
    int zoomTop = mZoomIn.getTop();
    return zoomTop != 0 && zoomTop <= compassOffset + mCompassHeight;
  }

  interface OnTranslationChangedListener
  {
    void onTranslationChanged(float translation);

    void onFadeInZoomButtons();

    void onFadeOutZoomButtons();
  }
}
