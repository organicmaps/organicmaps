package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.UiUtils;

public class TopPlacePageAnimationController extends BasePlacePageAnimationController
{
  public TopPlacePageAnimationController(@NonNull PlacePageView placePage)
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
      mDownY = event.getRawY();
      break;
    case MotionEvent.ACTION_MOVE:
      if (Math.abs(mDownY - event.getRawY()) > mTouchSlop)
        return true;
      break;
    }

    return false;
  }

  @Override
  protected void initGestureDetector()
  {
    // Gestures
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
            if (distanceY > 0)
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
        if (mPlacePage.getState() == State.FULL_PLACEPAGE)
          mPlacePage.setState(State.PREVIEW_ONLY);
        else
          mPlacePage.setState(State.FULL_PLACEPAGE);

        return true;
      }
    });
  }

  protected void showPlacePage(final boolean show)
  {
    if (mIsPlacePageVisible == show)
      return; // if state is already same as we need

    TranslateAnimation slide;
    if (show) // slide up
    {
      slide = UiUtils.generateRelativeSlideAnimation(0, 0, -1, 0);
      slide.setDuration(SHORT_ANIM_DURATION);
      UiUtils.show(mPlacePageDetails);
      //      UiUtils.hide(mArrow);
      if (mVisibilityChangedListener != null)
        mVisibilityChangedListener.onPlacePageVisibilityChanged(show);
    }
    else // slide down
    {
      slide = UiUtils.generateRelativeSlideAnimation(0, 0, 0, -1);

      slide.setDuration(SHORT_ANIM_DURATION);
      slide.setFillEnabled(true);
      slide.setFillBefore(true);
      //      UiUtils.show(mArrow);
      slide.setAnimationListener(new UiUtils.SimpleAnimationListener()
      {
        @Override
        public void onAnimationEnd(Animation animation)
        {
          UiUtils.hide(mPlacePageDetails);

          if (mVisibilityChangedListener != null)
            mVisibilityChangedListener.onPlacePageVisibilityChanged(show);
        }
      });
    }
    mPlacePageDetails.startAnimation(slide);

    mIsPlacePageVisible = show;
  }

  protected void showPreview(final boolean show)
  {
    if (mIsPreviewVisible == show)
      return;

    TranslateAnimation slide;
    if (show)
    {
      slide = UiUtils.generateRelativeSlideAnimation(0, 0, -1, 0);
      UiUtils.show(mPreview);
      slide.setAnimationListener(new UiUtils.SimpleAnimationListener()
      {
        @Override
        public void onAnimationEnd(Animation animation)
        {
          if (mVisibilityChangedListener != null)
            mVisibilityChangedListener.onPreviewVisibilityChanged(show);
        }
      });
    }
    else
    {
      slide = UiUtils.generateRelativeSlideAnimation(0, 0, 0, -1);
      slide.setAnimationListener(new UiUtils.SimpleAnimationListener()
      {
        @Override
        public void onAnimationEnd(Animation animation)
        {
          UiUtils.hide(mPreview);
          if (mVisibilityChangedListener != null)
            mVisibilityChangedListener.onPreviewVisibilityChanged(show);
        }
      });
    }
    slide.setDuration(SHORT_ANIM_DURATION);
    mPreview.startAnimation(slide);
    mIsPreviewVisible = show;
    //    UiUtils.show(mArrow);
  }

  protected void hidePlacePage()
  {
    final TranslateAnimation slideDown = UiUtils.generateRelativeSlideAnimation(0, 0, 0, -1);
    slideDown.setDuration(LONG_ANIM_DURATION);

    slideDown.setAnimationListener(new UiUtils.SimpleAnimationListener()
    {
      @Override
      public void onAnimationEnd(Animation animation)
      {
        mIsPlacePageVisible = false;
        mIsPreviewVisible = false;

        UiUtils.hide(mPreview);
        UiUtils.hide(mPlacePageDetails);
        if (mVisibilityChangedListener != null)
        {
          mVisibilityChangedListener.onPreviewVisibilityChanged(false);
          mVisibilityChangedListener.onPlacePageVisibilityChanged(false);
        }
      }
    });

    mPlacePage.startAnimation(slideDown);
  }
}
