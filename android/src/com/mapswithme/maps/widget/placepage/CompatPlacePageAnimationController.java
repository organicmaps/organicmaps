package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;

// TODO remove this class after minSdk will be 11+
public class CompatPlacePageAnimationController extends BasePlacePageAnimationController
{
  public CompatPlacePageAnimationController(@NonNull PlacePageView placePage)
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
      mDownCoord = event.getY() - ((ViewGroup) mPreview.getParent()).getTop();
      break;
    case MotionEvent.ACTION_MOVE:
      final float yDiff = mDownCoord - event.getY();
      final float buttonsY = mButtons.getTop();
      if (mDownCoord < mPreview.getTop() || mDownCoord > buttonsY ||
          (mDownCoord > mFrame.getTop() && mDownCoord < buttonsY))
        return false;
      if (Math.abs(yDiff) > mTouchSlop)
        return true;
      break;
    }

    return false;
  }

  @Override
  protected boolean onTouchEvent(@NonNull MotionEvent event)
  {
    if (mDownCoord < mPreview.getTop() || mDownCoord > mButtons.getTop())
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
        final float absDistanceY = Math.abs(distanceY);
        final boolean isVertical = absDistanceY > X_TO_Y_SCROLL_RATIO * Math.abs(distanceX);
        final boolean isInRange = absDistanceY > Y_MIN && absDistanceY < Y_MAX;

        if (isVertical && isInRange)
        {
          if (!mIsGestureHandled)
          {
            if (distanceY < 0f)
            {
              Framework.deactivatePopup();
              mPlacePage.setState(State.HIDDEN);
            }
            else
              mPlacePage.setState(State.DETAILS);

            mIsGestureHandled = true;
          }

          return true;
        }

        return false;
      }

      @Override
      public boolean onSingleTapConfirmed(MotionEvent e)
      {
        if (mDownCoord < mPreview.getTop() && mDownCoord < mFrame.getTop())
          return false;

        if (mPlacePage.getState() == State.PREVIEW)
          mPlacePage.setState(State.DETAILS);
        else
          mPlacePage.setState(State.PREVIEW);

        return true;
      }
    });
  }

  @Override
  void animateStateChange(State currentState, State newState)
  {
    switch (newState)
    {
    case HIDDEN:
      hidePlacePage();
      break;
    case PREVIEW:
      showPreview();
      break;
    case BOOKMARK:
      showBookmark();
      break;
    case DETAILS:
      showDetails();
      break;
    }
  }

  protected void showPreview()
  {
    mPlacePage.setVisibility(View.VISIBLE);
    mPreview.setVisibility(View.VISIBLE);
    mFrame.setVisibility(View.GONE);

    mIsPlacePageVisible = false;
    mIsPreviewVisible = true;
    notifyVisibilityListener();
  }

  protected void showDetails()
  {
    mPlacePage.setVisibility(View.VISIBLE);
    mPreview.setVisibility(View.VISIBLE);
    mFrame.setVisibility(View.VISIBLE);
    mBookmarkDetails.setVisibility(View.GONE);

    mIsPreviewVisible = mIsPlacePageVisible = true;
    notifyVisibilityListener();
  }

  void showBookmark()
  {
    mPlacePage.setVisibility(View.VISIBLE);
    mPreview.setVisibility(View.VISIBLE);
    mFrame.setVisibility(View.VISIBLE);
    mBookmarkDetails.setVisibility(View.VISIBLE);
    mButtons.setVisibility(View.VISIBLE);
    mButtons.bringToFront();

    mIsPreviewVisible = mIsPlacePageVisible = true;
    notifyVisibilityListener();
  }

  protected void hidePlacePage()
  {
    mPlacePage.setVisibility(View.GONE);
    mIsPreviewVisible = mIsPlacePageVisible = false;
    notifyVisibilityListener();
  }
}
