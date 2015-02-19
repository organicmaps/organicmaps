package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;

import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.UiUtils;

public class LeftFullAnimationController extends BasePlacePageAnimationController
{
  public LeftFullAnimationController(@NonNull PlacePageView placePage)
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
      mDownCoord = event.getRawX();
      break;
    case MotionEvent.ACTION_MOVE:
      if (mDownCoord > mPlacePage.getRight())
        return false;
      if (Math.abs(mDownCoord - event.getRawX()) > mTouchSlop)
        return true;
      break;
    }

    return false;
  }

  @Override
  protected boolean onTouchEvent(@NonNull MotionEvent event)
  {
    if (mDownCoord > mPlacePage.getRight())
      return false;

    super.onTouchEvent(event);
    return true;
  }

  @Override
  protected void initGestureDetector()
  {
    mGestureDetector = new GestureDetectorCompat(mPlacePage.getContext(), new GestureDetector.SimpleOnGestureListener()
    {
      private static final int X_MIN = 1;
      private static final int X_MAX = 100;
      private static final int X_TO_Y_SCROLL_RATIO = 2;

      @Override
      public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY)
      {
        final boolean isHorizontal = Math.abs(distanceX) > X_TO_Y_SCROLL_RATIO * Math.abs(distanceY);
        final boolean isInRange = Math.abs(distanceX) > X_MIN && Math.abs(distanceX) < X_MAX;

        if (isHorizontal && isInRange)
        {
          if (!mIsGestureHandled)
          {
            if (distanceX > 0)
              mPlacePage.setState(State.HIDDEN);

            mIsGestureHandled = true;
          }

          return true;
        }

        return false;
      }
    });
  }

  @Override
  protected void showPreview(final boolean show)
  {
    showPlacePage(show);
  }

  @Override
  protected void showPlacePage(final boolean show)
  {
    if (mIsPlacePageVisible == show)
      return;

    TranslateAnimation slide;
    if (show)
    {
      slide = UiUtils.generateRelativeSlideAnimation(-1, 0, 0, 0);
    }
    else
    {
      slide = UiUtils.generateRelativeSlideAnimation(0, -1, 0, 0);
      slide.setAnimationListener(new UiUtils.SimpleAnimationListener()
      {
        @Override
        public void onAnimationEnd(Animation animation)
        {
          mIsPlacePageVisible = false;
          mIsPreviewVisible = false;
          mPlacePage.setVisibility(View.GONE);
        }
      });
    }
    slide.setDuration(LONG_ANIM_DURATION);
    mPlacePage.startAnimation(slide);
    mPlacePage.setVisibility(View.VISIBLE);
    if (mVisibilityChangedListener != null)
    {
      mVisibilityChangedListener.onPlacePageVisibilityChanged(true);
      mIsPlacePageVisible = true;
    }
  }

  @Override
  protected void hidePlacePage()
  {
    showPlacePage(false);
  }
}
