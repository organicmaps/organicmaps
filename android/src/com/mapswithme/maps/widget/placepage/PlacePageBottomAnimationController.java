package com.mapswithme.maps.widget.placepage;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.annotation.SuppressLint;
import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.support.v7.widget.Toolbar;
import android.util.Log;
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
  private static final String TAG = PlacePageBottomAnimationController.class.getSimpleName();
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
      final float delta = mDownCoord - event.getY();
      if (Math.abs(delta) > mTouchSlop &&
              pressedInside(mDownCoord) &&
              !isDetailsScroll(mDownCoord, delta))
        return true;
      break;
    }

    return false;
  }

  private boolean pressedInside(float y)
  {
    return y > mPreview.getY() && y < mButtons.getY();
  }

  /**
   * @return whether gesture is scrolling of details content(and not dragging PP itself).
   */
  private boolean isDetailsScroll(float y, float delta)
  {
    return pressOnDetails(y) && isDetailsScrollable() && canScroll(delta);
  }

  private boolean pressOnDetails(float y)
  {
    return y > mDetailsFrame.getY() && y < mButtons.getY();
  }

  private boolean isDetailsScrollable()
  {
    return mDetailsFrame.getHeight() != mDetailsContent.getHeight();
  }

  private boolean canScroll(float delta)
  {
    return mDetailsScroll.getScrollY() != 0 || delta > 0;
  }

  @Override
  protected boolean onTouchEvent(@NonNull MotionEvent event)
  {
    // TODO test if that case is possible at all
    if (!pressedInside(mDownCoord))
      return false;

    Log.d(TAG, "onTouchEvent: " + event);
    if (event.getAction() == MotionEvent.ACTION_UP)
      finishAnimation();

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
          mIsGestureHandled = true;

        Log.d(TAG, "onScroll: " + e1 + ", e2 : " + e2 + ", y : " + distanceY);
        animateBy(distanceY);

        return mIsGestureHandled;
      }

      @Override
      public boolean onSingleTapUp(MotionEvent e)
      {
        Log.d(TAG, "onSingleTapUp: " + e);
        // TODO need to catch up gesture and finish animation there
        return super.onSingleTapUp(e);
      }

      @Override
      public boolean onSingleTapConfirmed(MotionEvent e)
      {
        if (mDownCoord < mPreview.getY() && mDownCoord < mDetailsFrame.getY())
          return false;

        mPlacePage.setState(mPlacePage.getState() == State.PREVIEW ? State.DETAILS
                                                                   : State.PREVIEW);

        return true;
      }
    });
  }

  private void finishAnimation()
  {
    final float currentTranslation = mDetailsFrame.getTranslationY();
    if (currentTranslation < 0)
    {
      hidePlacePage();
      return;
    }

    final float detailsHeight = mDetailsContent.getHeight();
    final float deltaTop = detailsHeight + currentTranslation;
    final float deltaBottom = -currentTranslation;

    if (deltaBottom > deltaTop)
      showDetails(mState);
    else
      showPreview(mState);
  }

  private void animateBy(float distanceY)
  {
    mPreview.setTranslationY(mPreview.getTranslationY() + distanceY);
    mDetailsFrame.setTranslationY(mDetailsFrame.getTranslationY() + distanceY);
  }

  @Override
  protected void onStateChanged(final State currentState, final State newState, @MapObject.MapObjectType int type)
  {
    prepareYTranslations(currentState, newState, type);

    mPlacePage.post(new Runnable()
    {
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
   *
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
        float translationY = (Float) animation.getAnimatedValue();
        mPreview.setTranslationY(translationY);
        mDetailsFrame.setTranslationY(translationY + detailsHeight);
        notifyProgress();
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

    ValueAnimator animator = ValueAnimator.ofFloat(0, -detailsScreenHeight);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        float translationY = (Float) animation.getAnimatedValue();
        mPreview.setTranslationY(translationY);
        mDetailsFrame.setTranslationY(translationY + detailsScreenHeight);
        notifyProgress();
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
        notifyProgress();
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

    final float animHeight = mPlacePage.getHeight() - mPreview.getY();
    final ValueAnimator animator = ValueAnimator.ofFloat(0f, animHeight);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        mPlacePage.setTranslationY((Float) animation.getAnimatedValue());
        notifyProgress();
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
        notifyProgress();
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

  private void notifyProgress()
  {
    notifyProgress(0, mPreview.getTranslationY());
  }
}
