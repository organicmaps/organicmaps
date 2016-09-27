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

class BottomPlacePageAnimationController extends BasePlacePageAnimationController
{
  @SuppressWarnings("unused")
  private static final String TAG = BottomPlacePageAnimationController.class.getSimpleName();
  private static final float DETAIL_RATIO = 0.7f;
  private static final float SWIPE_RATIO = 0.15f;
  private final ViewGroup mLayoutToolbar;

  private final AnimationHelper mAnimationHelper = new AnimationHelper();
  private ValueAnimator mCurrentAnimator;

  private boolean mIsGestureStartedInsideView;
  private boolean mIsGestureFinished;

  private float mDetailMaxHeight;
  private float mSwipeHeight;

  private class AnimationHelper
  {
    final View.OnLayoutChangeListener mListener = new View.OnLayoutChangeListener()
    {
      @Override
      public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight, int oldBottom)
      {
        if ((mState == State.DETAILS || mState == State.FULLSCREEN || mState == State.SCROLL)
                && v.getId() == mDetailsFrame.getId() && top != oldTop)
        {
          mPreview.setTranslationY(-mDetailsContent.getHeight());
          refreshToolbarVisibility();
        }
      }
    };
  }

  BottomPlacePageAnimationController(@NonNull PlacePageView placePage)
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

    float screenHeight = placePage.getResources().getDisplayMetrics().heightPixels;
    mDetailMaxHeight = screenHeight * DETAIL_RATIO;
    mSwipeHeight = screenHeight * SWIPE_RATIO;
  }

  @Override
  protected boolean onInterceptTouchEvent(MotionEvent event)
  {
    switch (event.getAction())
    {
    case MotionEvent.ACTION_DOWN:
      if (!isInsideView(event.getY()))
      {
        mIsGestureStartedInsideView = false;
        break;
      }

      mIsGestureStartedInsideView = true;
      mIsDragging = false;
      mIsGestureFinished = false;
      mDownCoord = event.getY();
      break;
    case MotionEvent.ACTION_MOVE:
      if (!mIsGestureStartedInsideView)
        break;

      final float delta = mDownCoord - event.getY();
      if (Math.abs(delta) > mTouchSlop && !isDetailsScroll(mDownCoord, delta))
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
    return mPlacePage.getState() == State.SCROLL
            && isDetailContentScrollable();
  }

  private boolean canScroll(float delta)
  {
    return mDetailsScroll.getScrollY() != 0 || delta > 0;
  }

  @Override
  protected boolean onTouchEvent(@NonNull MotionEvent event)
  {
    if (mIsGestureFinished)
      return false;

    final boolean finishedDrag = (mIsDragging &&
                                      (event.getAction() == MotionEvent.ACTION_UP ||
                                       event.getAction() == MotionEvent.ACTION_CANCEL));
    if (!mIsGestureStartedInsideView ||
        !isInsideView(event.getY()) ||
        finishedDrag)
    {
      mIsGestureFinished = true;
      finishDrag(mDownCoord - event.getY());
      return false;
    }

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
          mIsDragging = true;
          translateBy(-distanceY, e1.getY() - e2.getY());
        }

        return true;
      }

      @Override
      public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY)
      {
        if (velocityY > 0) {
          if (mPlacePage.getState() == State.HIDDEN) {
            mPlacePage.setState(State.DETAILS);
          } else if (isDetailContentScrollable()) {
            mPlacePage.setState(State.FULLSCREEN);
          }
        } else {
          if (mPlacePage.getState() == State.FULLSCREEN) {
            mPlacePage.setState(State.DETAILS);
          } else {
            mPlacePage.setState(State.HIDDEN);
          }
        }
        return super.onFling(e1, e2, velocityX, velocityY);
      }

      @Override
      public boolean onSingleTapConfirmed(MotionEvent e)
      {
        if (!isInPreview(e.getY()))
          return false;

        if (mPlacePage.getState() == State.PREVIEW) {
          if (isDetailContentScrollable()) {
            mPlacePage.setState(State.DETAILS);
          } else {
            mPlacePage.setState(State.SCROLL);
          }
        } else {
          mPlacePage.setState(State.PREVIEW);
        }

        return true;
      }
    });
  }

  private boolean isInPreview(float y)
  {
    return y > (mPreview.getTop() + mPreview.getTranslationY())
               && y < (mPreview.getBottom() + mPreview.getTranslationY());
  }

  private void finishDrag(float distance)
  {
    final float currentTranslation = mDetailsFrame.getTranslationY();
    if (currentTranslation > mDetailsScroll.getHeight())
    {
      mPlacePage.setState(State.HIDDEN);
      return;
    }

    boolean isSwipe = Math.abs(distance) > mSwipeHeight;
    if (!isDetailContentScrollable()
        && Math.abs(distance) >= mDetailsFrame.getHeight() * SWIPE_RATIO)
    {
      isSwipe = true;
    }

    if (distance >= 0.0f && isSwipe) {
      if (mPlacePage.getState() == State.PREVIEW) {
        if (isDetailContentScrollable()) {
          mPlacePage.setState(State.DETAILS);
        } else {
          mPlacePage.setState(State.SCROLL);
        }
      } else if (isDetailContentScrollable()) {
        if (mPlacePage.getState() == State.FULLSCREEN)
        {
          mPlacePage.setState(State.SCROLL);
        } else
        {
          mPlacePage.setState(State.FULLSCREEN);
        }
      } else if (mDetailsFrame.getTranslationY() > 0) {
        mPlacePage.setState(State.DETAILS);
      }
    } else if (isSwipe) {
      if (mPlacePage.getState() == State.SCROLL
          || mPlacePage.getState() == State.FULLSCREEN) {
        if (isDetailContentScrollable()) {
          mPlacePage.setState(State.DETAILS);
        } else {
          mPlacePage.setState(State.PREVIEW);
        }
      } else {
        mPlacePage.setState(State.PREVIEW);
      }
    } else {
      mPlacePage.setState(mPlacePage.getState());
    }
  }

  private boolean isDetailContentScrollable() {
    return mDetailsFrame.getHeight() < mDetailsContent.getHeight();
  }

  private void translateBy(float distanceY, float distanceFromBegin)
  {
    final float detailsHeight = mDetailsScroll.getHeight();
    float detailsTranslation = mDetailsFrame.getTranslationY() + distanceY;
    float previewTranslation = mPreview.getTranslationY() + distanceY;
    if (detailsTranslation < 0)
    {
      detailsTranslation = 0;
      previewTranslation = -detailsHeight;
      mIsDragging = false;
      mIsGestureFinished = true;
      finishDrag(distanceFromBegin);
    }

    mPreview.setTranslationY(previewTranslation);
    mDetailsFrame.setTranslationY(detailsTranslation);
    refreshToolbarVisibility();
  }

  @Override
  protected void onStateChanged(final State currentState, final State newState, @MapObject.MapObjectType int type)
  {
    prepareYTranslations(newState, type);

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
          showDetails();
          break;
        case FULLSCREEN:
          showFullscreen();
          break;
        case SCROLL:
          showScroll();
          break;
        }
      }
    });
  }

  /**
   * Prepares widgets for animating, places them vertically accordingly to their supposed positions.
   */
  private void prepareYTranslations(State newState, @MapObject.MapObjectType int type)
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

  private void showPreview(final State currentState)
  {
    if (mLayoutToolbar != null)
      UiUtils.hide(mLayoutToolbar);

    mDetailsFrame.addOnLayoutChangeListener(mAnimationHelper.mListener);

    mCurrentAnimator = ValueAnimator.ofFloat(mPreview.getTranslationY(), 0f);
    final float detailsHeight = mDetailsFrame.getHeight();
    mCurrentAnimator.addUpdateListener(new UpdateListener(detailsHeight));
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

  private void showDetails()
  {
    final float previewTranslation = mPreview.getHeight() + mButtons.getHeight();
    final float detailsScreenHeight = mDetailsScroll.getHeight();
    final float targetTranslation = Math
            .min(detailsScreenHeight, mDetailMaxHeight - previewTranslation);

    mCurrentAnimator = ValueAnimator.ofFloat(mPreview.getTranslationY(), -targetTranslation);
    mCurrentAnimator.addUpdateListener(new UpdateListener(detailsScreenHeight));
    mCurrentAnimator.addListener(new AnimationListener());

    startDefaultAnimator();
  }

  private void showFullscreen() {
    final float detailsScreenHeight = mDetailsScroll.getHeight();
    final float targetTranslation = -mPreview.getTop();

    if(!isDetailContentScrollable()) {
      mCurrentAnimator = ValueAnimator.ofFloat(mPreview.getTranslationY(), -detailsScreenHeight);
    } else {
      mCurrentAnimator = ValueAnimator.ofFloat(mPreview.getTranslationY(), targetTranslation);
    }
    mCurrentAnimator.addUpdateListener(new UpdateListener(detailsScreenHeight));
    mCurrentAnimator.addListener(new AnimationListener());

    startDefaultAnimator();
  }

  private void showScroll() {
    final float detailsScreenHeight = mDetailsScroll.getHeight();

    mCurrentAnimator = ValueAnimator.ofFloat(mPreview.getTranslationY(), -detailsScreenHeight);
    mCurrentAnimator.addUpdateListener(new UpdateListener(detailsScreenHeight));
    mCurrentAnimator.addListener(new AnimationListener());

    startDefaultAnimator();
  }

  @SuppressLint("NewApi")
  private void hidePlacePage()
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
      mCurrentAnimator = null;
    }
  }

  private void refreshToolbarVisibility()
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

  private class UpdateListener implements ValueAnimator.AnimatorUpdateListener
  {
    private final float mDetailsHeight;

    UpdateListener(float detailsHeight)
    {
      mDetailsHeight = detailsHeight;
    }

    @Override
    public void onAnimationUpdate(ValueAnimator animation)
    {
      float translationY = (Float) animation.getAnimatedValue();
      mPreview.setTranslationY(translationY);
      mDetailsFrame.setTranslationY(translationY + mDetailsHeight);
      notifyProgress();
    }
  }

  private class AnimationListener extends UiUtils.SimpleAnimatorListener {
    @Override
    public void onAnimationEnd(Animator animation)
    {
      refreshToolbarVisibility();
      notifyVisibilityListener(true, true);
      mDetailsScroll.scrollTo(0, 0);
      notifyProgress();
    }
  }
}
