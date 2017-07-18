package com.mapswithme.maps.widget.placepage;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.view.GestureDetector;
import android.view.MotionEvent;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.UiUtils;

class LeftPlacePageAnimationController extends BasePlacePageAnimationController
{
  LeftPlacePageAnimationController(@NonNull PlacePageView placePage)
  {
    super(placePage);
    UiUtils.extendViewPaddingWithStatusBar(mPreview);
  }

  @Override
  protected void initialVisibility()
  {
    UiUtils.hide(mPlacePage);
  }

  @Override
  protected boolean onInterceptTouchEvent(MotionEvent event)
  {
    if (mPlacePage.isHorizontalScrollAreaTouched(event))
      return false;

    switch (event.getAction())
    {
      case MotionEvent.ACTION_DOWN:
        mIsDragging = false;
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

        if (!isHorizontal || !isInRange)
          return false;

        if (!mIsDragging)
        {
          if (distanceX > 0)
            mPlacePage.hide();

          mIsDragging = true;
        }

        return true;
      }
    });
  }

  @Override
  protected void onStateChanged(State currentState, State newState, int type)
  {
    switch (newState)
    {
      case HIDDEN:
        hidePlacePage();
        break;
      case FULLSCREEN:
      case DETAILS:
      case PREVIEW:
        showPlacePage(currentState);
        break;
    }
  }

  @Override
  public void onScroll(int left, int top)
  {
    super.onScroll(left, top);
    notifyProgress(0, 0);
  }

  private void startTracking(boolean collapsed)
  {
    MwmActivity.LeftAnimationTrackListener tracker = mPlacePage.getLeftAnimationTrackListener();
    if (tracker != null)
      tracker.onTrackStarted(collapsed);

    notifyProgress(0, 0);
  }

  private void finishTracking(boolean collapsed)
  {
    MwmActivity.LeftAnimationTrackListener tracker = mPlacePage.getLeftAnimationTrackListener();
    if (tracker != null)
      tracker.onTrackFinished(collapsed);

    notifyProgress(0, 0);
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

    notifyProgress(0, 0);
  }

  private void showPlacePage(final State currentState)
  {
    UiUtils.show(mPlacePage);
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

    startDefaultAnimator(animator);
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

    startDefaultAnimator(animator);
  }
}
