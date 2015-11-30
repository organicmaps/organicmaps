package com.mapswithme.maps.widget.placepage;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.animation.AccelerateInterpolator;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.UiUtils;

class PlacePageLeftAnimationController extends BasePlacePageAnimationController
{
  public PlacePageLeftAnimationController(@NonNull PlacePageView placePage)
  {
    super(placePage);
  }

  @Override
  protected void initialHide()
  {
    UiUtils.hide(mPlacePage);
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
      private final int X_MIN = UiUtils.toPx(30);
      private final int X_MAX = UiUtils.toPx(100);
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
              mPlacePage.hide();

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

  private void startTracking(boolean collapsed)
  {
    MwmActivity.LeftAnimationTrackListener tracker = mPlacePage.getLeftAnimationTrackListener();
    if (tracker != null)
      tracker.onTrackStarted(collapsed);
  }

  private void finishTracking(boolean collapsed)
  {
    MwmActivity.LeftAnimationTrackListener tracker = mPlacePage.getLeftAnimationTrackListener();
    if (tracker != null)
      tracker.onTrackFinished(collapsed);
  }

  private void track(ValueAnimator animation)
  {
    float offset = (Float) animation.getAnimatedValue();
    mPlacePage.setTranslationX(offset);

    float slope = offset / mPlacePage.getDockedWidth();
    mPlacePage.setAlpha(slope + 1.0f);

    if (!mPlacePage.isDocked())
      mPlacePage.setRotation(slope * 20.0f);

    MwmActivity.LeftAnimationTrackListener tracker = mPlacePage.getLeftAnimationTrackListener();
    if (tracker != null)
      tracker.onTrackLeftAnimation(offset + mPlacePage.getDockedWidth());
  }

  private void showPlacePage(final State currentState, final State newState)
  {
    UiUtils.show(mPlacePage);
    UiUtils.showIf(newState == State.BOOKMARK, mBookmarkDetails);
    if (currentState != State.HIDDEN)
      return;

    startTracking(false);

    ValueAnimator animator = ValueAnimator.ofFloat(-mPlacePage.getDockedWidth(), 0.0f);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        track(animation);
      }
    });
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        notifyVisibilityListener(true, true);
        finishTracking(true);
      }
    });

    animator.setDuration(DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
  }

  private void hidePlacePage()
  {
    startTracking(true);

    ValueAnimator animator = ValueAnimator.ofFloat(0.0f, -mPlacePage.getDockedWidth());
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        track(animation);
      }
    });
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        UiUtils.hide(mPlacePage);
        notifyVisibilityListener(false, false);
        finishTracking(false);
      }
    });

    animator.setDuration(DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
  }
}
