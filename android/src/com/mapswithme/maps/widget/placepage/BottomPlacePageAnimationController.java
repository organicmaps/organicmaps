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

public class BottomPlacePageAnimationController extends BasePlacePageAnimationController
{
  public BottomPlacePageAnimationController(@NonNull PlacePageView placePage)
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
            if (distanceY < 0f)
            {
              if (mPlacePage.getState() == State.FULL_PLACEPAGE)
                mPlacePage.setState(State.PREVIEW_ONLY);
              else
              {
                mPlacePage.setState(State.HIDDEN);
                Framework.deactivatePopup();
              }
            }
            else
              mPlacePage.setState(State.FULL_PLACEPAGE);

            mIsGestureHandled = true;
          }

          return true;
        }

        return false;
      }

      @Override
      public boolean onSingleTapConfirmed(MotionEvent e)
      {
        if (mDownCoord < mPreview.getY() && mDownCoord < mDetails.getY())
          return false;

        if (mPlacePage.getState() == State.FULL_PLACEPAGE)
          mPlacePage.setState(State.PREVIEW_ONLY);
        else
          mPlacePage.setState(State.FULL_PLACEPAGE);

        return true;
      }
    });
  }

  @Override
  protected void showPreview(final boolean show)
  {
    mPlacePage.setVisibility(View.VISIBLE);
    mPreview.setVisibility(View.VISIBLE);
    mDetails.setVisibility(View.INVISIBLE);
    if (mIsPreviewVisible == show)
      return;

    ValueAnimator animator;
    if (show)
    {
      animator = ValueAnimator.ofFloat(mPreview.getHeight(), 0f);
      animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
      {
        @Override
        public void onAnimationUpdate(ValueAnimator animation)
        {
          ViewHelper.setTranslationY(mPreview, (Float) animation.getAnimatedValue());
          ViewHelper.setTranslationY(mButtons, (Float) animation.getAnimatedValue());
        }
      });
    }
    else
    {
      animator = ValueAnimator.ofFloat(0, mPreview.getHeight());
      animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
      {
        @Override
        public void onAnimationUpdate(ValueAnimator animation)
        {
          ViewHelper.setTranslationY(mPreview, (Float) animation.getAnimatedValue());
          ViewHelper.setTranslationY(mButtons, (Float) animation.getAnimatedValue());

          if (1f - animation.getAnimatedFraction() < 0.01)
          {
            mPlacePage.setVisibility(View.INVISIBLE);
            mPlacePage.setMapObject(null);
          }
        }
      });
    }
    animator.setDuration(SHORT_ANIM_DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();

    mIsPreviewVisible = show;
    if (mVisibilityChangedListener != null)
      mVisibilityChangedListener.onPreviewVisibilityChanged(mIsPreviewVisible);
  }

  @Override
  protected void showPlacePage(final boolean show)
  {
    mPlacePage.setVisibility(View.VISIBLE);
    mDetails.setVisibility(View.VISIBLE);
    if (mIsPlacePageVisible == show)
      return; // if state is already same as we need

    ValueAnimator animator;
    final float animHeight = mDetails.getHeight();
    if (show)
    {
      animator = ValueAnimator.ofFloat(animHeight, 0f);
      animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
      {
        @Override
        public void onAnimationUpdate(ValueAnimator animation)
        {
          ViewHelper.setTranslationY(mDetails, (Float) animation.getAnimatedValue());
          ViewHelper.setTranslationY(mPreview, (Float) animation.getAnimatedValue() - animHeight);
        }
      });
    }
    else
    {
      animator = ValueAnimator.ofFloat(0, animHeight);
      animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
      {
        @Override
        public void onAnimationUpdate(ValueAnimator animation)
        {
          ViewHelper.setTranslationY(mDetails, (Float) animation.getAnimatedValue());
          ViewHelper.setTranslationY(mPreview, (Float) animation.getAnimatedValue() - animHeight);

          if (1f - animation.getAnimatedFraction() < 0.01)
          {
            mDetails.setVisibility(View.INVISIBLE);
            ViewHelper.setTranslationY(mPreview, 0f);
          }
        }
      });
    }
    animator.setDuration(SHORT_ANIM_DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();

    mIsPlacePageVisible = show;
    if (mVisibilityChangedListener != null)
      mVisibilityChangedListener.onPlacePageVisibilityChanged(mIsPlacePageVisible);
  }

  @Override
  protected void hidePlacePage()
  {
    final float animHeight = mPlacePage.getHeight() - mPreview.getTop() - ViewHelper.getTranslationY(mPreview);
    final ValueAnimator animator = ValueAnimator.ofFloat(0, animHeight);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        ViewHelper.setTranslationY(mPlacePage, (Float) animation.getAnimatedValue());

        if (1f - animation.getAnimatedFraction() < 0.01)
        {
          mIsPlacePageVisible = false;
          mIsPreviewVisible = false;

          mPlacePage.setVisibility(View.INVISIBLE);
          ViewHelper.setTranslationY(mPlacePage, 0f);
          if (mVisibilityChangedListener != null)
          {
            mVisibilityChangedListener.onPreviewVisibilityChanged(false);
            mVisibilityChangedListener.onPlacePageVisibilityChanged(false);
          }
        }
      }
    });
    animator.setDuration(LONG_ANIM_DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
  }
}
