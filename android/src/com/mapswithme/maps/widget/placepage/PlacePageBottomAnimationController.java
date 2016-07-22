package com.mapswithme.maps.widget.placepage;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.annotation.SuppressLint;
import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.support.v7.widget.Toolbar;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Interpolator;
import android.view.animation.OvershootInterpolator;
import android.widget.LinearLayout;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.concurrency.UiThread;

class PlacePageBottomAnimationController extends BasePlacePageAnimationController
{
  private final ViewGroup mLayoutToolbar;

  private final AnimationHelper mAnimationHelper = new AnimationHelper();

  private class AnimationHelper
  {
    final View.OnLayoutChangeListener mListener = new View.OnLayoutChangeListener()
    {
      @Override
      public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight, int oldBottom)
      {
        if (mState == State.DETAILS && v.getId() == mDetailsFrame.getId() && top != oldTop)
        {
          mPreview.setTranslationY(-mDetailsContent.getHeight());
          refreshToolbarVisibility();
        }
      }
    };
  }

  public PlacePageBottomAnimationController(@NonNull PlacePageView placePage)
  {
    super(placePage);
    mLayoutToolbar = (LinearLayout) mPlacePage.findViewById(R.id.toolbar_layout);
    if (mLayoutToolbar == null)
      return;

    final Toolbar toolbar = (Toolbar) mLayoutToolbar.findViewById(R.id.toolbar);
    if (toolbar != null)
    {
      UiUtils.showHomeUpButton(toolbar);
      toolbar.setNavigationOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          mPlacePage.hide();
        }
      });
    }
  }

  @Override
  protected boolean onInterceptTouchEvent(MotionEvent event)
  {
    switch (event.getAction())
    {
    case MotionEvent.ACTION_DOWN:
      mIsGestureHandled = false;
      mDownCoord = event.getY();
      break;
    case MotionEvent.ACTION_MOVE:
      final float yDiff = mDownCoord - event.getY();
      if (mDownCoord < mPreview.getY() || mDownCoord > mButtons.getY() ||
              (mDownCoord > mDetailsFrame.getY() && mDownCoord < mButtons.getY() &&
                   (mDetailsFrame.getHeight() != mDetailsContent.getHeight() && (mDetailsScroll.getScrollY() != 0 || yDiff > 0))))
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
      private final int Y_MIN = UiUtils.toPx(10);
      private final int Y_MAX = UiUtils.toPx(50);
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
              mPlacePage.hide();
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
        if (mDownCoord < mPreview.getY() && mDownCoord < mDetailsFrame.getY())
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
  protected void onStateChanged(final State currentState, final State newState, @MapObject.MapObjectType int type)
  {
    prepareYTranslations(currentState, newState, type);

    mPlacePage.post(new Runnable() {
      @Override
      public void run()
      {
        switch (newState)
        {
        case HIDDEN:
          hidePlacePage();
          break;
        case PREVIEW:
          showPreview(currentState);
          break;
        case DETAILS:
          showDetails(currentState);
          break;
        }
      }
    });
  }

  /**
   * Prepares widgets for animating, places them vertically accordingly to their supposed positions.
   * @param currentState
   * @param newState
   * @param type
   */
  private void prepareYTranslations(State currentState, State newState, @MapObject.MapObjectType int type)
  {
    switch (newState)
    {
    case PREVIEW:
      if (mState == State.HIDDEN)
      {
        UiUtils.invisible(mPlacePage, mPreview, mDetailsFrame, mButtons);
        UiUtils.showIf(type == MapObject.BOOKMARK, mBookmarkDetails);
        mPlacePage.post(new Runnable()
        {
          @Override
          public void run()
          {
            final float previewTranslation = mPreview.getHeight() + mButtons.getHeight();
            mPreview.setTranslationY(previewTranslation);
            mDetailsFrame.setTranslationY(mDetailsFrame.getHeight());
            mButtons.setTranslationY(previewTranslation);

            UiUtils.show(mPlacePage, mPreview, mButtons);
          }
        });
      }
      break;
    case DETAILS:
      UiUtils.show(mPlacePage, mPreview, mDetailsFrame);
      UiUtils.showIf(type == MapObject.BOOKMARK, mBookmarkDetails);
      break;
    }
  }

  protected void showPreview(final State currentState)
  {
    if (mLayoutToolbar != null)
      UiUtils.hide(mLayoutToolbar);

    mDetailsFrame.addOnLayoutChangeListener(mAnimationHelper.mListener);

    ValueAnimator animator = ValueAnimator.ofFloat(mPreview.getTranslationY(), 0f);
    final float detailsHeight = mDetailsFrame.getHeight();
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        mPreview.setTranslationY((Float) animation.getAnimatedValue());
        mDetailsFrame.setTranslationY((Float) animation.getAnimatedValue() + detailsHeight);
      }
    });
    Interpolator interpolator;
    if (currentState == State.HIDDEN)
    {
      interpolator = new OvershootInterpolator();
      animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
      {
        @Override
        public void onAnimationUpdate(ValueAnimator animation)
        {
          mButtons.setTranslationY((Float) animation.getAnimatedValue());
        }
      });
    }
    else
    {
      interpolator = new AccelerateInterpolator();
    }

    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        notifyVisibilityListener(true, false);
      }
    });

    startDefaultAnimator(animator, interpolator);
  }

  protected void showDetails(final State currentState)
  {
    if (currentState != State.PREVIEW)
      return;

    final float detailsScreenHeight = mDetailsScroll.getHeight();

    ValueAnimator animator = ValueAnimator.ofFloat(detailsScreenHeight, 0);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        mPreview.setTranslationY((Float) animation.getAnimatedValue() - detailsScreenHeight);
        mDetailsFrame.setTranslationY((Float) animation.getAnimatedValue());
      }
    });
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        refreshToolbarVisibility();
        notifyVisibilityListener(true, true);
        mDetailsScroll.scrollTo(0, 0);
      }
    });

    startDefaultAnimator(animator);
  }

  @SuppressLint("NewApi")
  protected void hidePlacePage()
  {
    if (mLayoutToolbar != null)
      UiUtils.hide(mLayoutToolbar);

    mDetailsFrame.removeOnLayoutChangeListener(mAnimationHelper.mListener);

    final float animHeight = mPlacePage.getHeight() - mPreview.getTop() - mPreview.getTranslationY();
    final ValueAnimator animator = ValueAnimator.ofFloat(0f, animHeight);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        mPlacePage.setTranslationY((Float) animation.getAnimatedValue());
      }
    });
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        initialVisibility();
        mPlacePage.setTranslationY(0);
        notifyVisibilityListener(false, false);
      }
    });

    startDefaultAnimator(animator);
  }

  protected void refreshToolbarVisibility()
  {
    if (mLayoutToolbar != null)
      UiThread.runLater(new Runnable()
      {
        @Override
        public void run()
        {
          UiUtils.showIf(mPreview.getY() < 0, mLayoutToolbar);
        }
      });
  }
}
