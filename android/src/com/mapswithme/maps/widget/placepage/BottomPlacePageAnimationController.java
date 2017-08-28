package com.mapswithme.maps.widget.placepage;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.annotation.SuppressLint;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.view.GestureDetectorCompat;
import android.support.v7.widget.Toolbar;
import android.util.DisplayMetrics;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Interpolator;
import android.widget.LinearLayout;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.widget.ObservableLinearLayout;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.concurrency.UiThread;

class BottomPlacePageAnimationController extends BasePlacePageAnimationController
    implements ObservableLinearLayout.SizeChangedListener
{
  @SuppressWarnings("unused")
  private static final String TAG = BottomPlacePageAnimationController.class.getSimpleName();
  private static final float DETAIL_RATIO = 0.7f;
  private static final float SCROLL_DELTA = 50.0f;
  private final ViewGroup mLayoutToolbar;

  private ValueAnimator mCurrentAnimator;

  private boolean mIsGestureStartedInsideView;
  private boolean mIsGestureFinished;
  private boolean mIsHiding;

  private float mScreenHeight;
  private float mDetailMaxHeight;
  private float mScrollDelta;
  private int mContentHeight;

  @Nullable
  private OnBannerOpenListener mBannerOpenListener;

  BottomPlacePageAnimationController(@NonNull PlacePageView placePage)
  {
    super(placePage);
    UiUtils.extendViewMarginWithStatusBar(mDetailsScroll);
    mLayoutToolbar = (LinearLayout) mPlacePage.findViewById(R.id.toolbar_layout);
    if (mLayoutToolbar == null)
      return;

    ((ObservableLinearLayout) mDetailsContent).setSizeChangedListener(this);
    mContentHeight = mDetailsContent.getHeight();

    final Toolbar toolbar = (Toolbar) mLayoutToolbar.findViewById(R.id.toolbar);
    if (toolbar != null)
    {
      UiUtils.extendViewWithStatusBar(toolbar);
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

    DisplayMetrics dm = placePage.getResources().getDisplayMetrics();
    mScreenHeight = dm.heightPixels;
    mDetailMaxHeight = mScreenHeight * DETAIL_RATIO;
    mScrollDelta = SCROLL_DELTA * dm.density;
  }

  @Override
  protected boolean onInterceptTouchEvent(MotionEvent event)
  {
    if (mIsHiding)
      return false;

    switch (event.getAction())
    {
      case MotionEvent.ACTION_DOWN:
        if (!UiUtils.isViewTouched(event, mDetailsScroll))
        {
          mIsGestureStartedInsideView = false;
          break;
        }

        mIsGestureStartedInsideView = true;
        mIsDragging = false;
        mIsGestureFinished = false;
        mDownCoord = event.getY();
        mGestureDetector.onTouchEvent(event);
        break;
      case MotionEvent.ACTION_MOVE:
        if (!mIsGestureStartedInsideView)
          break;

        final float delta = mDownCoord - event.getY();
        if (Math.abs(delta) > mTouchSlop && !isDetailsScroll(delta))
          return true;

        break;
      case MotionEvent.ACTION_UP:
        final boolean isInside = UiUtils.isViewTouched(event, mDetailsScroll);
        final boolean isBannerTouch = mPlacePage.isBannerTouched(event);
        if (isInside && !isBannerTouch && mIsGestureStartedInsideView)
          mGestureDetector.onTouchEvent(event);
        mIsDragging = false;
        break;
      case MotionEvent.ACTION_CANCEL:
        mIsDragging = false;
        break;
    }

    return false;
  }

  @Override
  protected boolean onTouchEvent(@NonNull MotionEvent event)
  {
    if (mIsGestureFinished || mIsHiding)
      return false;

    final boolean finishedDrag = (mIsDragging &&
                                  (event.getAction() == MotionEvent.ACTION_UP ||
                                   event.getAction() == MotionEvent.ACTION_CANCEL));
    if (!mIsGestureStartedInsideView || !UiUtils.isViewTouched(event, mDetailsScroll)
        || finishedDrag)
    {
      mIsGestureFinished = true;
      finishDrag(mDownCoord - event.getY());
      return false;
    }

    super.onTouchEvent(event);
    return true;
  }

  @Override
  public void onScroll(int left, int top)
  {
    super.onScroll(left, top);
    resetDetailsScrollIfNeeded();
    refreshToolbarVisibility();
    notifyProgress();
  }

  private void resetDetailsScrollIfNeeded()
  {
    if (mCurrentScrollY > 0 && mDetailsScroll.getTranslationY() > 0)
    {
      mDetailsScroll.scrollTo(0, 0);
    }
  }

  /**
   * @return whether gesture is scrolling of details content(and not dragging PP itself).
   */
  private boolean isDetailsScroll(float delta)
  {
    return isDetailsScrollable() && canScroll(delta);
  }

  private boolean isDetailsScrollable()
  {
    return mPlacePage.getState() == State.FULLSCREEN && isDetailContentScrollable();
  }

  private boolean isDetailContentScrollable()
  {
    return mDetailsScroll.getHeight() < mContentHeight;
  }

  private boolean canScroll(float delta)
  {
    return mDetailsScroll.getScrollY() != 0 || delta > 0;
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
        if (mIsHiding)
          return true;

        final boolean isVertical = Math.abs(distanceY) > X_TO_Y_SCROLL_RATIO * Math.abs(distanceX);

        if (isVertical)
        {
          mIsDragging = true;
          if (!translateBy(-distanceY))
          {
            boolean scrollable = isDetailContentScrollable();
            int maxTranslationY = mDetailsScroll.getHeight() - mContentHeight;
            if ((scrollable && mDetailsScroll.getTranslationY() == 0)
                || (!scrollable && mDetailsScroll.getTranslationY() <= maxTranslationY))
            {
              mDetailsScroll.scrollBy((int) distanceX, (int) distanceY);
              mState = State.FULLSCREEN;
            }
          }
        }

        return true;
      }

      @Override
      public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY)
      {
        if (mIsHiding)
          return true;

        finishDrag(-velocityY);
        return true;
      }

      @Override
      public boolean onSingleTapConfirmed(MotionEvent e)
      {
        if (mIsHiding)
          return true;

        MotionEvent evt = MotionEvent.obtain(e.getDownTime(),
                                             e.getEventTime(),
                                             e.getAction(),
                                             e.getX(),
                                             mDownCoord,
                                             e.getMetaState());
        if (UiUtils.isViewTouched(evt, mPreview))
        {
          if (mPlacePage.getState() == State.PREVIEW)
          {
            if (isDetailContentScrollable())
            {
              mPlacePage.setState(State.DETAILS);
            }
            else
            {
              mPlacePage.setState(State.FULLSCREEN);
            }
          }
          else
          {
            mPlacePage.setState(State.PREVIEW);
          }
        }
        return true;
      }
    });
    mDetailsScroll.setGestureDetector(mGestureDetector);
  }

  private void finishDrag(float distance)
  {
    mIsDragging = false;
    final float currentTranslation = mDetailsScroll.getTranslationY();
    if (currentTranslation > mDetailsScroll.getHeight())
    {
      mPlacePage.setState(State.HIDDEN);
      return;
    }

    boolean isDragUp = distance >= 0.0f;

    if (isDragUp && mPlacePage.getState() == State.FULLSCREEN)
      return;

    if (isDragUp) // drag up
    {
      if (mPlacePage.getState() == State.PREVIEW)
      {
        if (isDetailContentScrollable())
        {
          mPlacePage.setState(State.DETAILS);
        }
        else
        {
          mPlacePage.setState(State.FULLSCREEN);
        }
      }
      else if (mPlacePage.getState() != State.FULLSCREEN)
      {
        mPlacePage.setState(State.FULLSCREEN);
      }
      else if (mCurrentScrollY == 0 || mDetailsScroll.getTranslationY() > 0)
      {
        mPlacePage.setState(State.DETAILS);
      }
    }
    else // drag down
    {
      if (mPlacePage.getState() == State.FULLSCREEN)
      {
        if (isDetailContentScrollable())
        {
          mPlacePage.setState(State.DETAILS);
        }
        else
        {
          mPlacePage.setState(State.PREVIEW);
        }
      }
      else if (mPlacePage.getState() == State.DETAILS)
      {
        mPlacePage.setState(State.PREVIEW);
      }
      else
      {
        mPlacePage.setState(State.HIDDEN);
      }
    }
  }

  private boolean translateBy(float distanceY)
  {
    if (mCurrentScrollY > 0)
      return false;

    if (Math.abs(distanceY) > mScrollDelta)
      distanceY = 0.0f;
    float detailsTranslation = mDetailsScroll.getTranslationY() + distanceY;
    final boolean isScrollable = isDetailContentScrollable();
    boolean consumeEvent = true;
    final float maxTranslationY = mDetailsScroll.getHeight() - mContentHeight;
    if ((isScrollable && detailsTranslation < 0.0f) || detailsTranslation < maxTranslationY)
    {
      if (isScrollable)
      {
        detailsTranslation = 0.0f;
        mDetailsScroll.setGestureDetector(null);
      }
      else
      {
        detailsTranslation = maxTranslationY;
      }

      mState = State.FULLSCREEN;
      consumeEvent = false;
    }

    mDetailsScroll.setTranslationY(detailsTranslation);
    notifyProgress();
    refreshToolbarVisibility();

    if (mBannerOpenListener != null
        && detailsTranslation < mDetailsScroll.getHeight() - mPreview.getHeight())
    {
      mBannerOpenListener.onNeedOpenBanner();
    }
    return consumeEvent;
  }

  @Override
  protected void onStateChanged(final State currentState, final State newState, @MapObject.MapObjectType int type)
  {
    if (newState == State.HIDDEN && currentState == newState)
      return;

    prepareYTranslations(newState, type);

    mDetailsScroll.setGestureDetector(mGestureDetector);
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
            if (isDetailContentScrollable())
              mDetailsScroll.setGestureDetector(null);
            showFullscreen();
            break;
        }
      }
    });
  }

  /**
   * Prepares widgets for animating, places them vertically accordingly to their supposed
   * positions.
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
              mDetailsScroll.setTranslationY(mDetailsScroll.getHeight());
              mButtons.setTranslationY(mPreview.getHeight() + mButtons.getHeight());

              UiUtils.show(mPlacePage, mPreview, mButtons, mDetailsFrame);
            }
          });
        }
        break;
      case FULLSCREEN:
      case DETAILS:
        UiUtils.show(mPlacePage, mPreview, mButtons, mDetailsFrame);
        UiUtils.showIf(type == MapObject.BOOKMARK, mBookmarkDetails);
        break;
    }
  }

  private void showPreview(final State currentState)
  {
    if (mLayoutToolbar != null)
      UiUtils.hide(mLayoutToolbar);

    final float translation = mDetailsScroll.getHeight() - mPreview.getHeight();
    mCurrentAnimator = ValueAnimator.ofFloat(mDetailsScroll.getTranslationY(), translation);
    mCurrentAnimator.addUpdateListener(new UpdateListener());
    Interpolator interpolator = new AccelerateInterpolator();
    if (currentState == State.HIDDEN)
    {
      ValueAnimator buttonAnimator = ValueAnimator.ofFloat(mButtons.getTranslationY(), 0);
      buttonAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
      {
        @Override
        public void onAnimationUpdate(ValueAnimator animation)
        {
          mButtons.setTranslationY((Float) animation.getAnimatedValue());
        }
      });
      startDefaultAnimator(buttonAnimator, interpolator);
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
    final float translation;
    if (isDetailContentScrollable())
      translation = mDetailMaxHeight - mButtons.getHeight();
    else
      translation = mContentHeight > mDetailMaxHeight ? mDetailMaxHeight - mButtons.getHeight()
                                                      : mContentHeight;
    mCurrentAnimator = ValueAnimator.ofFloat(mDetailsScroll.getTranslationY(),
                                             mDetailsScroll.getHeight() - translation);
    mCurrentAnimator.addUpdateListener(new UpdateListener());
    mCurrentAnimator.addListener(new AnimationListener());

    startDefaultAnimator();
  }

  private void showFullscreen()
  {
    if (isDetailContentScrollable())
    {
      mCurrentAnimator = ValueAnimator.ofFloat(mDetailsScroll.getTranslationY(), 0);
    }
    else
    {
      mCurrentAnimator = ValueAnimator.ofFloat(mDetailsScroll.getTranslationY(),
                                               mDetailsScroll.getHeight() - mContentHeight);
    }
    mCurrentAnimator.addUpdateListener(new UpdateListener());
    mCurrentAnimator.addListener(new AnimationListener());

    startDefaultAnimator();
  }

  @SuppressLint("NewApi")
  private void hidePlacePage()
  {
    mIsHiding = true;
    if (mLayoutToolbar != null)
      UiUtils.hide(mLayoutToolbar);

    final float animHeight = mPlacePage.getHeight() - mPreview.getY();
    mCurrentAnimator = ValueAnimator.ofFloat(0f, animHeight);
    mCurrentAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        // FIXME: This translation adds a weird jumping up at end of PP closing
        mPlacePage.setTranslationY((Float) animation.getAnimatedValue());
        notifyProgress();
      }
    });
    mCurrentAnimator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        mIsHiding = false;
        initialVisibility();
        mPlacePage.setTranslationY(0);
        mDetailsScroll.setTranslationY(mDetailsScroll.getHeight());
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
          UiUtils.showIf(mCurrentScrollY > 0, mLayoutToolbar);
        }
      });
  }

  private void notifyProgress()
  {
    notifyProgress(0, mDetailsScroll.getTranslationY() + mPlacePage.getTranslationY());
  }

  void setBannerOpenListener(@Nullable OnBannerOpenListener bannerOpenListener)
  {
    mBannerOpenListener = bannerOpenListener;
  }

  private void onContentSizeChanged()
  {
    if (mIsDragging || mCurrentScrollY > 0)
      return;

    MapObject object = mPlacePage.getMapObject();
    if (object != null)
      onStateChanged(getState(), getState(), object.getMapObjectType());
  }

  @Override
  public void onSizeChanged(int width, int height, int oldWidth, int oldHeight)
  {
    mContentHeight = height;
    onContentSizeChanged();
  }

  private class UpdateListener implements ValueAnimator.AnimatorUpdateListener
  {
    @Override
    public void onAnimationUpdate(ValueAnimator animation)
    {
      float translationY = (Float) animation.getAnimatedValue();
      mDetailsScroll.setTranslationY(translationY);
      notifyProgress();
    }
  }

  private class AnimationListener extends UiUtils.SimpleAnimatorListener
  {
    @Override
    public void onAnimationEnd(Animator animation)
    {
      refreshToolbarVisibility();
      notifyVisibilityListener(true, true);
      mDetailsScroll.scrollTo(0, 0);
      notifyProgress();
    }
  }

  interface OnBannerOpenListener
  {
    void onNeedOpenBanner();
  }
}
