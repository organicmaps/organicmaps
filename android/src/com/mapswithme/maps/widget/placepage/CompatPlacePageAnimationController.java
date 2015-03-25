package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.support.v7.widget.Toolbar;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Interpolator;
import android.widget.RelativeLayout;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.UiUtils;
import com.nineoldandroids.animation.ValueAnimator;
import com.nineoldandroids.view.ViewHelper;

// TODO remove this class after minSdk will be 11+
public class CompatPlacePageAnimationController extends BasePlacePageAnimationController
{
  private final Toolbar mToolbar;

  public CompatPlacePageAnimationController(@NonNull PlacePageView placePage)
  {
    super(placePage);
    mToolbar = (Toolbar) mPlacePage.findViewById(R.id.toolbar);
    if (mToolbar != null)
    {
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
      final float buttonsY = ViewHelper.getY(mButtons);
      if (mDownCoord < ViewHelper.getY(mPreview) || mDownCoord > buttonsY ||
          (mDownCoord > ViewHelper.getY(mDetails) && mDownCoord < buttonsY &&
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
        final float absDistanceY = Math.abs(distanceY);
        final boolean isVertical = absDistanceY > X_TO_Y_SCROLL_RATIO * Math.abs(distanceX);
        final boolean isInRange = absDistanceY > Y_MIN && absDistanceY < Y_MAX;

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
    if (mToolbar != null)
      mToolbar.setVisibility(View.GONE);

    ValueAnimator animator;
    Interpolator interpolator;
    if (currentState == State.HIDDEN)
    {
      setMargin(mPreview, 0f);
      setMargin(mDetails, 0f);
      mDetails.setVisibility(View.GONE);
      interpolator = new AccelerateInterpolator();
      animator = ValueAnimator.ofFloat(mPreview.getHeight() + mButtons.getHeight(), 0f);
      animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
      {
        @Override
        public void onAnimationUpdate(ValueAnimator animation)
        {
          setMargin(mButtons, -((Float) animation.getAnimatedValue()));
          setMargin(mPreview, -((Float) animation.getAnimatedValue()));

          if (isAnimationCompleted(animation))
          {
            mIsPlacePageVisible = false;
            mIsPreviewVisible = true;
            notifyVisibilityListener();
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
          setMargin(mPreview, -(Float) animation.getAnimatedValue());
          setMargin(mDetails, -(Float) animation.getAnimatedValue() - detailsHeight);

          if (isAnimationCompleted(animation))
          {
            mDetails.setVisibility(View.GONE);
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

  private void setMargin(ViewGroup view, Float margin)
  {
    final RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) view.getLayoutParams();
    params.bottomMargin = margin.intValue();
    view.setLayoutParams(params);
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

    animator.addUpdateListener(createDetailsUpdateListener(detailsHeight, animator));

    animator.setDuration(SHORT_ANIM_DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
  }

  private ValueAnimator.AnimatorUpdateListener createDetailsUpdateListener(final float detailsHeight, final ValueAnimator animator)
  {
    return new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        setMargin(mPreview, detailsHeight - (Float) animation.getAnimatedValue());
        setMargin(mDetails, -(Float) animation.getAnimatedValue());

        if (isAnimationCompleted(animation))
        {
          refreshToolbarVisibility();
          mIsPreviewVisible = mIsPlacePageVisible = true;
          notifyVisibilityListener();
        }
      }
    };
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
    animator.addUpdateListener(createDetailsUpdateListener(detailsHeight, animator));

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

    final float animHeight = mPlacePage.getHeight() - mPreview.getTop() - ViewHelper.getTranslationY(mPreview);
    final ValueAnimator animator = ValueAnimator.ofFloat(0f, animHeight);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        setMargin(mPlacePage, -(Float) animation.getAnimatedValue());


        if (isAnimationCompleted(animation))
        {
          mIsPreviewVisible = mIsPlacePageVisible = false;
          setMargin(mPlacePage, 0f);
          setMargin(mPreview, 0f);
          setMargin(mDetails, 0f);
          notifyVisibilityListener();
          mPlacePage.setVisibility(View.GONE);
        }
      }
    });
    animator.setDuration(SHORT_ANIM_DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
  }
}
