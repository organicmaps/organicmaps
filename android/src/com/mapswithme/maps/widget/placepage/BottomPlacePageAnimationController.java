package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.support.v7.widget.Toolbar;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Interpolator;
import android.view.animation.OvershootInterpolator;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.UiUtils;
import com.nineoldandroids.animation.ValueAnimator;
import com.nineoldandroids.view.ViewHelper;

public class BottomPlacePageAnimationController extends BasePlacePageAnimationController implements View.OnLayoutChangeListener
{
  private int mDetailsTop;
  private final View mViewBottomHack;
  private final Toolbar mToolbar;

  public BottomPlacePageAnimationController(@NonNull PlacePageView placePage)
  {
    super(placePage);
    mViewBottomHack = mPlacePage.findViewById(R.id.view_bottom_white);
    mToolbar = (Toolbar) mPlacePage.findViewById(R.id.toolbar);
    if (mToolbar != null)
    {
      mDetailsTop = mToolbar.getHeight();
      UiUtils.showHomeUpButton(mToolbar);
      mToolbar.setNavigationOnClickListener(new View.OnClickListener()
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
  boolean onInterceptTouchEvent(MotionEvent event)
  {
    switch (event.getAction())
    {
    case MotionEvent.ACTION_DOWN:
      mIsGestureHandled = false;
      mDownCoord = event.getY();
      break;
    case MotionEvent.ACTION_MOVE:
      final float yDiff = mDownCoord - event.getY();
      if (mDownCoord < ViewHelper.getY(mPreview) || mDownCoord > ViewHelper.getY(mButtons) ||
          (mDownCoord > ViewHelper.getY(mDetails) && mDownCoord < ViewHelper.getY(mButtons) &&
              (mDetails.getHeight() != mDetails.getChildAt(0).getHeight() && (mDetails.getScrollY() != 0 || yDiff > 0))))
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
    if (mDownCoord < ViewHelper.getY(mPreview) || mDownCoord > ViewHelper.getY(mButtons))
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
        final boolean isVertical = Math.abs(distanceY) > X_TO_Y_SCROLL_RATIO * Math.abs(distanceX);
        final boolean isInRange = Math.abs(distanceY) > Y_MIN && Math.abs(distanceY) < Y_MAX;

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
        if (mDownCoord < ViewHelper.getY(mPreview) && mDownCoord < ViewHelper.getY(mDetails))
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
    mPlacePage.setVisibility(View.VISIBLE);
    mPreview.setVisibility(View.VISIBLE);
    mDetails.addOnLayoutChangeListener(this);
    if (mToolbar != null)
      mToolbar.setVisibility(View.GONE);

    ValueAnimator animator;
    Interpolator interpolator;
    if (currentState == State.HIDDEN)
    {
      mViewBottomHack.setVisibility(View.GONE);
      mDetails.setVisibility(View.INVISIBLE);
      interpolator = new OvershootInterpolator();
      animator = ValueAnimator.ofFloat(mPreview.getHeight() + mButtons.getHeight(), 0f);
      animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
      {
        @Override
        public void onAnimationUpdate(ValueAnimator animation)
        {
          ViewHelper.setTranslationY(mPreview, (Float) animation.getAnimatedValue());
          ViewHelper.setTranslationY(mButtons, (Float) animation.getAnimatedValue());

          final float fraction = animation.getAnimatedFraction();
          if (fraction > .5f)
          {
            mViewBottomHack.setVisibility(View.VISIBLE);
            if (animation.getAnimatedFraction() > .99f)
            {
              mIsPlacePageVisible = false;
              mIsPreviewVisible = true;
              notifyVisibilityListener();
            }
          }
        }
      });
    }
    else
    {
      final float detailsHeight = mDetails.getHeight();
      interpolator = new AccelerateInterpolator();
      animator = ValueAnimator.ofFloat(ViewHelper.getTranslationY(mPreview), 0f);
      animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
      {
        @Override
        public void onAnimationUpdate(ValueAnimator animation)
        {
          ViewHelper.setTranslationY(mPreview, (Float) animation.getAnimatedValue());
          ViewHelper.setTranslationY(mDetails, (Float) animation.getAnimatedValue() + detailsHeight);

          if (animation.getAnimatedFraction() > .99f)
          {
            mDetails.setVisibility(View.INVISIBLE);
            mIsPlacePageVisible = false;
            mIsPreviewVisible = true;
            notifyVisibilityListener();
          }
        }
      });
    }
    animator.setDuration(SHORT_ANIM_DURATION);
    animator.setInterpolator(interpolator);
    animator.start();
  }

  protected void showDetails(final State currentState)
  {
    mPlacePage.setVisibility(View.VISIBLE);
    mPreview.setVisibility(View.VISIBLE);
    mDetails.setVisibility(View.VISIBLE);

    ValueAnimator animator;
    final float bookmarkHeight = mBookmarkDetails.getHeight();
    final float detailsHeight = mDetails.getHeight();
    if (currentState == State.PREVIEW)
      animator = ValueAnimator.ofFloat(detailsHeight, bookmarkHeight);
    else
      animator = ValueAnimator.ofFloat(0f, bookmarkHeight);

    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        ViewHelper.setTranslationY(mPreview, (Float) animation.getAnimatedValue() - detailsHeight);
        ViewHelper.setTranslationY(mDetails, (Float) animation.getAnimatedValue());

        if (animation.getAnimatedFraction() > .99f)
        {
          refreshToolbarVisibility();
          mIsPreviewVisible = mIsPlacePageVisible = true;
          notifyVisibilityListener();
        }
      }
    });

    animator.setDuration(SHORT_ANIM_DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
  }

  void showBookmark(final State currentState)
  {
    mPlacePage.setVisibility(View.VISIBLE);
    mPreview.setVisibility(View.VISIBLE);
    mDetails.setVisibility(View.VISIBLE);
    mBookmarkDetails.setVisibility(View.VISIBLE);

    ValueAnimator animator;
    final float bookmarkHeight = mBookmarkDetails.getHeight();
    final float detailsHeight = mDetails.getHeight();

    if (currentState == State.DETAILS)
      animator = ValueAnimator.ofFloat(bookmarkHeight, 0f);
    else
      animator = ValueAnimator.ofFloat(detailsHeight, 0f);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        ViewHelper.setTranslationY(mPreview, (Float) animation.getAnimatedValue() - detailsHeight);
        ViewHelper.setTranslationY(mDetails, (Float) animation.getAnimatedValue());

        if (animation.getAnimatedFraction() > .99f)
        {
          refreshToolbarVisibility();
          mIsPreviewVisible = mIsPlacePageVisible = true;
          notifyVisibilityListener();
        }
      }
    });

    animator.setDuration(SHORT_ANIM_DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
  }

  private void refreshToolbarVisibility()
  {
    if (mToolbar != null)
      mToolbar.setVisibility(ViewHelper.getY(mDetails) < mPreview.getHeight() ? View.VISIBLE : View.GONE);
  }

  protected void hidePlacePage()
  {
    if (mToolbar != null)
      mToolbar.setVisibility(View.GONE);

    mDetails.removeOnLayoutChangeListener(this);
    final float animHeight = mPlacePage.getHeight() - mPreview.getTop() - ViewHelper.getTranslationY(mPreview);
    final ValueAnimator animator = ValueAnimator.ofFloat(0f, animHeight);
    mViewBottomHack.setVisibility(View.GONE);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        ViewHelper.setTranslationY(mPlacePage, (Float) animation.getAnimatedValue());

        if (animation.getAnimatedFraction() > .99f)
        {
          mIsPreviewVisible = mIsPlacePageVisible = false;

          mPlacePage.setVisibility(View.INVISIBLE);
          ViewHelper.setTranslationY(mPlacePage, 0);
          notifyVisibilityListener();
        }
      }
    });
    animator.setDuration(SHORT_ANIM_DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
  }

  @Override
  public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight, int oldBottom)
  {
    if (mDetails.getVisibility() == View.VISIBLE && v.getId() == mDetails.getId() && top != oldTop)
    {
      ViewHelper.setTranslationY(mPreview, -mDetails.getHeight());
      refreshToolbarVisibility();
    }
  }
}
