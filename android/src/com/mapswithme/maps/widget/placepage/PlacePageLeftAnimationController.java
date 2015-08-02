package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.animation.AccelerateInterpolator;

import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.UiUtils;
import com.nineoldandroids.animation.Animator;
import com.nineoldandroids.animation.ValueAnimator;
import com.nineoldandroids.view.ViewHelper;

class PlacePageLeftAnimationController extends BasePlacePageAnimationController
{
  public PlacePageLeftAnimationController(@NonNull PlacePageView placePage)
  {
    super(placePage);
  }

  @Override
  protected boolean onInterceptTouchEvent(MotionEvent event)
  {
    switch (event.getAction())
    {
    case MotionEvent.ACTION_DOWN:
      mIsGestureHandled = false;
      mDownCoord = event.getX();
      break;
    case MotionEvent.ACTION_MOVE:
      if (mDownCoord > mPlacePage.getRight())
        return false;
      if (Math.abs(mDownCoord - event.getX()) > mTouchSlop)
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
      private static final int X_MIN = 4;
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
  public void setState(State state, MapObject.MapObjectType type)
  {
    if (state == State.PREVIEW && type == MapObject.MapObjectType.BOOKMARK)
      state = State.BOOKMARK;

    super.setState(state, type);
  }

  @Override
  protected void onStateChanged(State currentState, State newState)
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
    UiUtils.show(mPlacePage);
    UiUtils.showIf(newState == State.BOOKMARK, mBookmarkDetails);
    if (currentState != State.HIDDEN)
      return;

    if (NO_ANIMATION)
    {
      finishAnimation(true, true);
      return;
    }

    ValueAnimator animator = ValueAnimator.ofFloat(-mPlacePage.getWidth(), 0f);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        ViewHelper.setTranslationX(mPlacePage, (Float) animation.getAnimatedValue());
      }
    });
    animator.addListener(new UiUtils.SimpleNineoldAnimationListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        finishAnimation(true, true);
      }
    });

    animator.setDuration(DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
  }

  protected void hidePlacePage()
  {
    if (NO_ANIMATION)
    {
      UiUtils.hide(mPlacePage);
      finishAnimation(false, false);
      return;
    }

    ValueAnimator animator;
    animator = ValueAnimator.ofFloat(0.0f, -mPlacePage.getWidth());
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        ViewHelper.setTranslationX(mPlacePage, (Float) animation.getAnimatedValue());
      }
    });
    animator.addListener(new UiUtils.SimpleNineoldAnimationListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        UiUtils.invisible(mPlacePage);
        finishAnimation(false, false);
      }
    });

    animator.setDuration(DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
  }
}
