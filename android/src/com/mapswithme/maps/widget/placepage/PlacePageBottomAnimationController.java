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
        if (mState == State.BOOKMARK && v.getId() == mFrame.getId() && top != oldTop)
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
          mPlacePage.setState(State.HIDDEN);
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
          (mDownCoord > mFrame.getY() && mDownCoord < mButtons.getY() &&
              (mFrame.getHeight() != mDetailsContent.getHeight() && (mDetails.getScrollY() != 0 || yDiff > 0))))
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
              mPlacePage.setState(State.HIDDEN);
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
        if (mDownCoord < mPreview.getY() && mDownCoord < mFrame.getY())
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
  protected void onStateChanged(State currentState, State newState)
  {
    switch (newState)
    {
    case HIDDEN:
      hidePlacePage();
      break;
    case PREVIEW:
      showPreview(currentState);
      break;
    case BOOKMARK:
      showBookmark(currentState);
      break;
    case DETAILS:
      showDetails(currentState);
      break;
    }
  }

  protected void showPreview(final State currentState)
  {
    UiUtils.show(mPlacePage, mPreview);
    if (mLayoutToolbar != null)
      UiUtils.hide(mLayoutToolbar);

    mFrame.addOnLayoutChangeListener(mAnimationHelper.mListener);

    ValueAnimator animator;
    Interpolator interpolator;
    if (currentState == State.HIDDEN)
    {
      UiUtils.invisible(mFrame);

      interpolator = new OvershootInterpolator();
      animator = ValueAnimator.ofFloat(mPreview.getHeight() + mButtons.getHeight(), 0f);
      animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
      {
        @Override
        public void onAnimationUpdate(ValueAnimator animation)
        {
          mPreview.setTranslationY((Float) animation.getAnimatedValue());
          mButtons.setTranslationY((Float) animation.getAnimatedValue());
        }
      });
      animator.addListener(new UiUtils.SimpleAnimatorListener()
      {
        @Override
        public void onAnimationEnd(Animator animation)
        {
          notifyVisibilityListener(true, false);
        }
      });
    }
    else
    {
      final float detailsHeight = mFrame.getHeight();
      interpolator = new AccelerateInterpolator();
      animator = ValueAnimator.ofFloat(mPreview.getTranslationY(), 0f);
      animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
      {
        @Override
        public void onAnimationUpdate(ValueAnimator animation)
        {
          mPreview.setTranslationY((Float) animation.getAnimatedValue());
          mFrame.setTranslationY((Float) animation.getAnimatedValue() + detailsHeight);
        }
      });
      animator.addListener(new UiUtils.SimpleAnimatorListener()
      {
        @Override
        public void onAnimationEnd(Animator animation)
        {
          UiUtils.invisible(mFrame, mBookmarkDetails);
          notifyVisibilityListener(true, false);
        }
      });
    }
    animator.setDuration(DURATION);
    animator.setInterpolator(interpolator);
    animator.start();
  }

  private void showPreviewFrame()
  {
    UiUtils.show(mPlacePage, mPreview, mFrame);
  }

  protected void showDetails(final State currentState)
  {
    showPreviewFrame();

    final float detailsFullHeight = mDetailsContent.getHeight();
    final float detailsScreenHeight = mDetails.getHeight();
    final float bookmarkFullHeight = mBookmarkDetails.getHeight();
    final float bookmarkScreenHeight = bookmarkFullHeight - (detailsFullHeight - detailsScreenHeight);

    ValueAnimator animator = ValueAnimator.ofFloat(currentState == State.PREVIEW ? detailsScreenHeight : 0,
                                                   bookmarkScreenHeight);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        mPreview.setTranslationY((Float) animation.getAnimatedValue() - detailsScreenHeight);
        mFrame.setTranslationY((Float) animation.getAnimatedValue());
      }
    });
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        refreshToolbarVisibility();
        notifyVisibilityListener(true, true);
        mDetails.scrollTo(0, 0);
        UiUtils.invisible(mBookmarkDetails);
      }
    });

    animator.setDuration(DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
  }

  void showBookmark(final State currentState)
  {
    UiUtils.show(mBookmarkDetails);
    showPreviewFrame();

    final float detailsFullHeight = mDetailsContent.getHeight();
    final float detailsScreenHeight = mDetails.getHeight();
    final float bookmarkHeight = mBookmarkDetails.getHeight();

    ValueAnimator animator = ValueAnimator.ofFloat(currentState == State.DETAILS ? bookmarkHeight - (detailsFullHeight - detailsScreenHeight)
                                                                                 : detailsScreenHeight, 0.0f);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        mPreview.setTranslationY((Float) animation.getAnimatedValue() - detailsScreenHeight);
        mFrame.setTranslationY((Float) animation.getAnimatedValue());
      }
    });
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        refreshToolbarVisibility();
        notifyVisibilityListener(true, true);
      }
    });

    animator.setDuration(DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
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

  @SuppressLint("NewApi")
  protected void hidePlacePage()
  {
    if (mLayoutToolbar != null)
      UiUtils.hide(mLayoutToolbar);

    mFrame.removeOnLayoutChangeListener(mAnimationHelper.mListener);

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
        UiUtils.invisible(mPlacePage, mBookmarkDetails);
        mPlacePage.setTranslationY(0);
        notifyVisibilityListener(false, false);
      }
    });
    animator.setDuration(DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
  }
}
