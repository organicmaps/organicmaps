package com.mapswithme.maps;

import android.content.res.Resources;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.Animations;
import com.mapswithme.util.UiUtils;

class NavigationButtonsAnimationController
{
  @NonNull
  private final View mZoomIn;
  @NonNull
  private final View mZoomOut;
  @NonNull
  private final View mMyPosition;
  @Nullable
  private final View mCenter;

  private final float mMargin;
  private float mBottom;
  private float mTop;

  private boolean mIsZoomAnimate;
  private boolean mIsMyPosAnimate;

  NavigationButtonsAnimationController(@NonNull View zoomIn, @NonNull View zoomOut,
                                       @NonNull View myPosition, @Nullable View center)
  {
    mZoomIn = zoomIn;
    mZoomOut = zoomOut;
    mMyPosition = myPosition;
    mCenter = center;
    Resources res = mZoomIn.getResources();
    mMargin = res.getDimension(R.dimen.nav_button_top_limit);
    calculateLimitTranslations();
  }

  private void calculateLimitTranslations()
  {
    mTop = mMargin;
    mMyPosition.addOnLayoutChangeListener(new View.OnLayoutChangeListener()
    {
      @Override
      public void onLayoutChange(View v, int left, int top, int right, int bottom,
                                 int oldLeft, int oldTop, int oldRight, int oldBottom)
      {
        mBottom = bottom;
        mMyPosition.removeOnLayoutChangeListener(this);
      }
    });
  }

  void setTopLimit(int limit)
  {
    mTop = limit + mMargin;
  }

  private void fadeOutZoom()
  {
    mIsZoomAnimate = true;
    Animations.fadeOutView(mZoomIn, new Runnable()
    {
      @Override
      public void run()
      {
        mZoomIn.setVisibility(View.INVISIBLE);
        mIsZoomAnimate = false;
      }
    });
    Animations.fadeOutView(mZoomOut, new Runnable()
    {
      @Override
      public void run()
      {
        mZoomOut.setVisibility(View.INVISIBLE);
      }
    });
  }

  private void fadeInZoom()
  {
    mIsZoomAnimate = true;
    mZoomIn.setVisibility(View.VISIBLE);
    mZoomOut.setVisibility(View.VISIBLE);
    Animations.fadeInView(mZoomIn, new Runnable()
    {
      @Override
      public void run()
      {
        mIsZoomAnimate = false;
      }
    });
    Animations.fadeInView(mZoomOut, null);
  }

  private void fadeOutMyPosition()
  {
    mIsMyPosAnimate = true;
    Animations.fadeOutView(mMyPosition, new Runnable()
    {
      @Override
      public void run()
      {
        UiUtils.invisible(mMyPosition);
        mIsMyPosAnimate = false;
      }
    });
  }

  private void fadeInMyPosition()
  {
    mIsMyPosAnimate = true;
    mMyPosition.setVisibility(View.VISIBLE);
    Animations.fadeInView(mMyPosition, new Runnable()
    {
      @Override
      public void run()
      {
        mIsMyPosAnimate = false;
      }
    });
  }

  void onPlacePageMoved(float translationY)
  {
    if (mCenter == null || mBottom == 0)
      return;

    final float translation = translationY - mBottom;
    update(translation <= 0 ? translation : 0);
  }

  void update()
  {
    update(mZoomIn.getTranslationY());
  }

  private void update(final float translation)
  {
    mMyPosition.setTranslationY(translation);
    mZoomOut.setTranslationY(translation);
    mZoomIn.setTranslationY(translation);
    if (!mIsZoomAnimate && isOverTopLimit(mZoomIn))
    {
      fadeOutZoom();
    }
    else if (!mIsZoomAnimate && satisfyTopLimit(mZoomIn))
    {
      fadeInZoom();
    }

    if (!shouldBeHidden() && !mIsMyPosAnimate
        && isOverTopLimit(mMyPosition))
    {
      fadeOutMyPosition();
    }
    else if (!shouldBeHidden() && !mIsMyPosAnimate
             && satisfyTopLimit(mMyPosition))
    {
      fadeInMyPosition();
    }
  }

  private boolean isOverTopLimit(@NonNull View view)
  {
    return view.getVisibility() == View.VISIBLE && view.getY() <= mTop;
  }

  private boolean satisfyTopLimit(@NonNull View view)
  {
    return view.getVisibility() == View.INVISIBLE && view.getY() >= mTop;
  }

  private boolean shouldBeHidden()
  {
    return LocationState.getMode() == LocationState.FOLLOW_AND_ROTATE
           && (RoutingController.get().isPlanning() || RoutingController.get().isNavigating());
  }
}
