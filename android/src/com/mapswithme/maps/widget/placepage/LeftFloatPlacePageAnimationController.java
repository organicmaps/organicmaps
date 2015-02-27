package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.AccelerateInterpolator;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.nineoldandroids.animation.ValueAnimator;
import com.nineoldandroids.view.ViewHelper;

public class LeftFloatPlacePageAnimationController extends BasePlacePageAnimationController
{
  public LeftFloatPlacePageAnimationController(@NonNull PlacePageView placePage)
  {
    super(placePage);
  }

  @Override
  boolean onInterceptTouchEvent(MotionEvent event)
  {
    switch (event.getAction())
    {
    case MotionEvent.ACTION_DOWN:
      mIsGestureHandled = false;
      mDownCoord = event.getRawY();
      break;
    case MotionEvent.ACTION_MOVE:
      if (mDownCoord < mPreview.getY() || mDownCoord > mButtons.getY())
        return false;
      if (Math.abs(mDownCoord - event.getRawY()) > mTouchSlop)
        return true;
      break;
    }

    return false;
  }

  @Override
  protected boolean onTouchEvent(@NonNull MotionEvent event)
  {
    if (mDownCoord < mPreview.getY() || mDownCoord > mButtons.getY())
      return false;

    super.onTouchEvent(event);
    return true;
  }

  @Override
  protected void initGestureDetector()
  {
    mGestureDetector = new GestureDetectorCompat(mPlacePage.getContext(), new GestureDetector.SimpleOnGestureListener()
    {
      private static final int Y_MIN = 1;
      private static final int Y_MAX = 100;
      private static final int X_TO_Y_SCROLL_RATIO = 2;

      @Override
      public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY)
      {
        final boolean isVertical = Math.abs(distanceY) > X_TO_Y_SCROLL_RATIO * Math.abs(distanceX);
        final boolean isInRange = Math.abs(distanceY) > Y_MIN && Math.abs(distanceY) < Y_MAX;

        if (isVertical && isInRange)
        {
          if (!mIsGestureHandled)
          {
            if (distanceY < 0)
            {
              mPlacePage.setState(State.HIDDEN);
              Framework.deactivatePopup();
            }

            mIsGestureHandled = true;
          }

          return true;
        }

        return false;
      }
    });
  }

  @Override
  void setState(State currentState, State newState)
  {
    switch (newState)
    {
    case HIDDEN:
      hidePlacePage();
      break;
    case BOOKMARK:
    case DETAILS:
    case PREVIEW:
      showPlacePage(currentState, newState);
      break;
    }
  }

  protected void showPlacePage(final State currentState, final State newState)
  {
    mPlacePage.setVisibility(View.VISIBLE);
    mBookmarkDetails.setVisibility(newState == State.BOOKMARK ? View.VISIBLE : View.GONE);
    if (currentState == State.HIDDEN)
    {
      ValueAnimator animator = ValueAnimator.ofFloat(mPlacePage.getHeight(), 0f);
      animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
      {
        @Override
        public void onAnimationUpdate(ValueAnimator animation)
        {
          ViewHelper.setTranslationY(mPlacePage, (Float) animation.getAnimatedValue());
        }
      });

      animator.setDuration(SHORT_ANIM_DURATION);
      animator.setInterpolator(new AccelerateInterpolator());
      animator.start();
    }
    mVisibilityChangedListener.onPlacePageVisibilityChanged(true);
    mVisibilityChangedListener.onPreviewVisibilityChanged(true);
  }

  protected void hidePlacePage()
  {
    ValueAnimator animator;
    animator = ValueAnimator.ofFloat(0, mPlacePage.getHeight());
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        ViewHelper.setTranslationY(mPlacePage, (Float) animation.getAnimatedValue());

        if (animation.getAnimatedFraction() > 0.99f)
        {
          mPlacePage.setVisibility(View.INVISIBLE);
          mVisibilityChangedListener.onPlacePageVisibilityChanged(false);
          mVisibilityChangedListener.onPreviewVisibilityChanged(false);
        }
      }
    });

    animator.setDuration(SHORT_ANIM_DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
  }
}
