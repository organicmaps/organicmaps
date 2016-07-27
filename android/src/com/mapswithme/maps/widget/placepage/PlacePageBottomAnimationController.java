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
  private static final String TAG = PlacePageBottomAnimationController.class.getSimpleName();
  private final ViewGroup mLayoutToolbar;

  private final AnimationHelper mAnimationHelper = new AnimationHelper();
  private ValueAnimator mCurrentAnimator;

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
      if (!isInsideView(event.getY()))
        break;

      mIsGestureHandled = false;
      mDownCoord = event.getY();
      break;
    case MotionEvent.ACTION_MOVE:
      final float delta = mDownCoord - event.getY();
      if (Math.abs(delta) > mTouchSlop
              && !isDetailsScroll(mDownCoord, delta)
              && isInsideView(mDownCoord))
        return true;
      break;
    }

    return false;
  }

  private boolean isInsideView(float y)
  {
    return y > mPreview.getY() && y < mButtons.getY();
  }

  /**
   * @return whether gesture is scrolling of details content(and not dragging PP itself).
   */
  private boolean isDetailsScroll(float y, float delta)
  {
    return isOnDetails(y) && isDetailsScrollable() && canScroll(delta);
  }

  private boolean isOnDetails(float y)
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
    if (!isInsideView(event.getY()))
      return false;

    if (event.getAction() == MotionEvent.ACTION_UP)
      finishDrag();

    super.onTouchEvent(event);
    return true;
  }

  @Override
  protected void initGestureDetector()
  {
    mGestureDetector = new GestureDetectorCompat(mPlacePage.getContext(), new GestureDetector.SimpleOnGestureListener()
    {
      private static final int X_TO_Y_SCROLL_RATIO = 2;

      @Override
      public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY)
      {
        final boolean isVertical = Math.abs(distanceY) > X_TO_Y_SCROLL_RATIO * Math.abs(distanceX);

        if (isVertical)
        {
          mIsGestureHandled = true;
          translateBy(-distanceY);
        }

        return mIsGestureHandled;
      }

      @Override
      public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY)
      {
        mPlacePage.setState(velocityY > 0 ? State.HIDDEN : State.DETAILS);
        return super.onFling(e1, e2, velocityX, velocityY);
      }

      @Override
      public boolean onSingleTapConfirmed(MotionEvent e)
      {
        if (!isInPreview(e.getY()))
          return false;

        mPlacePage.setState(mPlacePage.getState() == State.PREVIEW ? State.DETAILS
                                                                   : State.PREVIEW);

        return true;
      }
    });
  }

  private boolean isInPreview(float y)
  {
    return y > (mPreview.getTop() + mPreview.getTranslationY())
               && y < (mPreview.getBottom() + mPreview.getTranslationY());
  }

  private void finishDrag()
  {
    final float currentTranslation = mDetailsFrame.getTranslationY();
    if (currentTranslation > mDetailsScroll.getHeight())
    {
      mPlacePage.setState(State.HIDDEN);
      return;
    }

    @SuppressWarnings("UnnecessaryLocalVariable")
    final float deltaTop = currentTranslation;
    final float deltaBottom = mDetailsContent.getHeight() - currentTranslation;

    mPlacePage.setState(deltaBottom > deltaTop ? State.DETAILS
                                               : State.PREVIEW);
  }

  private void translateBy(float distanceY)
  {
    final float detailsHeight = mDetailsScroll.getHeight();
    float detailsTranslation = mDetailsFrame.getTranslationY() + distanceY;
    float previewTranslation = mPreview.getTranslationY() + distanceY;
    if (detailsTranslation < 0)
    {
      detailsTranslation = 0;
      previewTranslation = -detailsHeight;
    }

    mPreview.setTranslationY(previewTranslation);
    mDetailsFrame.setTranslationY(detailsTranslation);
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
        endRunningAnimation();
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

            UiUtils.show(mPlacePage, mPreview, mButtons, mDetailsFrame);
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

    mCurrentAnimator = ValueAnimator.ofFloat(mPreview.getTranslationY(), 0f);
    final float detailsHeight = mDetailsFrame.getHeight();
    mCurrentAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
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
      mCurrentAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
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

    mCurrentAnimator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        notifyVisibilityListener(true, false);
      }
    });

    startDefaultAnimator(mCurrentAnimator, interpolator);
  }

  protected void showDetails(final State currentState)
  {
    final float detailsScreenHeight = mDetailsScroll.getHeight();

    mCurrentAnimator = ValueAnimator.ofFloat(mPreview.getTranslationY(), -detailsScreenHeight);
    mCurrentAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
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
    mCurrentAnimator.addListener(new UiUtils.SimpleAnimatorListener()
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

    startDefaultAnimator();
  }

  @SuppressLint("NewApi")
  protected void hidePlacePage()
  {
    if (mLayoutToolbar != null)
      UiUtils.hide(mLayoutToolbar);

    mDetailsFrame.removeOnLayoutChangeListener(mAnimationHelper.mListener);

    final float animHeight = mPlacePage.getHeight() - mPreview.getY();
    mCurrentAnimator = ValueAnimator.ofFloat(0f, animHeight);
    mCurrentAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        mPlacePage.setTranslationY((Float) animation.getAnimatedValue());
        notifyProgress();
      }
    });
    mCurrentAnimator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        initialVisibility();
        mPlacePage.setTranslationY(0);
        notifyProgress();
        notifyVisibilityListener(false, false);
      }
    });

    startDefaultAnimator();
  }

  private void startDefaultAnimator()
  {
    startDefaultAnimator(mCurrentAnimator);
  }

  private void endRunningAnimation()
  {
    if (mCurrentAnimator != null && mCurrentAnimator.isRunning())
    {
      mCurrentAnimator.removeAllUpdateListeners();
      mCurrentAnimator.removeAllListeners();
      mCurrentAnimator.end();
    }
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
