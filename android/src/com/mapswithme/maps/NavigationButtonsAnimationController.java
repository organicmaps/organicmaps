package com.mapswithme.maps;

import android.content.res.Resources;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.widget.menu.MyPositionButton;
import com.mapswithme.util.Animations;
import com.mapswithme.util.UiUtils;

class NavigationButtonsAnimationController
{
  @NonNull
  private final View mNavZoomIn;
  @NonNull
  private final View mNavZoomOut;
  @NonNull
  private final MyPositionButton mNavMyPosition;
  @Nullable
  private final View mCenter;

  private final float mScreenHeight;
  private final float mMargin;
  private float mBottom;
  private float mTop;

  private boolean mIsZoomAnimate;
  private boolean mIsMyPosAnimate;

  NavigationButtonsAnimationController(@NonNull View navZoomIn, @NonNull View navZoomOut,
                                       @NonNull MyPositionButton navMyPosition, @Nullable View center)
  {
    mNavZoomIn = navZoomIn;
    mNavZoomOut = navZoomOut;
    mNavMyPosition = navMyPosition;
    mCenter = center;
    Resources res = mNavZoomIn.getResources();
    mScreenHeight = res.getDisplayMetrics().heightPixels;
    mMargin = res.getDimension(R.dimen.margin_base_plus);
    calculateLimitTranslations();
  }

  private void calculateLimitTranslations()
  {
    mTop = mMargin;
    mNavMyPosition.getButton().addOnLayoutChangeListener(new View.OnLayoutChangeListener()
    {
      @Override
      public void onLayoutChange(View v, int left, int top, int right, int bottom,
                                 int oldLeft, int oldTop, int oldRight, int oldBottom)
      {
        mBottom = bottom + mMargin;
        mNavMyPosition.getButton().removeOnLayoutChangeListener(this);
      }
    });
  }

  void setTopLimit(float limit)
  {
    mTop = limit + mMargin;
  }

  private void fadeOutZoom()
  {
    mIsZoomAnimate = true;
    Animations.fadeOutView(mNavZoomIn, new Runnable()
    {
      @Override
      public void run()
      {
        mNavZoomIn.setVisibility(View.INVISIBLE);
        mIsZoomAnimate = false;
      }
    });
    Animations.fadeOutView(mNavZoomOut, new Runnable()
    {
      @Override
      public void run()
      {
        mNavZoomOut.setVisibility(View.INVISIBLE);
      }
    });
  }

  private void fadeInZoom()
  {
    mIsZoomAnimate = true;
    mNavZoomIn.setVisibility(View.VISIBLE);
    mNavZoomOut.setVisibility(View.VISIBLE);
    Animations.fadeInView(mNavZoomIn, new Runnable()
    {
      @Override
      public void run()
      {
        mIsZoomAnimate = false;
      }
    });
    Animations.fadeInView(mNavZoomOut, null);
  }

  private void fadeOutMyPosition()
  {
    mIsMyPosAnimate = true;
    Animations.fadeOutView(mNavMyPosition.getButton(), new Runnable()
    {
      @Override
      public void run()
      {
        UiUtils.invisible(mNavMyPosition.getButton());
        mIsMyPosAnimate = false;
      }
    });
  }

  private void fadeInMyPosition()
  {
    mIsMyPosAnimate = true;
    mNavMyPosition.getButton().setVisibility(View.VISIBLE);
    Animations.fadeInView(mNavMyPosition.getButton(), new Runnable()
    {
      @Override
      public void run()
      {
        mIsMyPosAnimate = false;
      }
    });
  }

  void onPlacePageMoved(float translationY, int baseTranslationY)
  {
    if (mCenter == null || mBottom == 0)
      return;

    final float shift = mScreenHeight - baseTranslationY;
    final float minTranslation = mScreenHeight - mBottom - shift;
    final float translation = minTranslation - (baseTranslationY - translationY);
    if (translation <= 0)
      update(translation);
    else
      update(0);
  }

  void update()
  {
    update(mNavZoomIn.getTranslationY());
  }

  private void update(final float translation)
  {
    mNavMyPosition.getButton().setTranslationY(translation);
    mNavZoomOut.setTranslationY(translation);
    mNavZoomIn.setTranslationY(translation);
    if (!mIsZoomAnimate && isOverTopLimit(mNavZoomIn))
    {
      fadeOutZoom();
    }
    else if (!mIsZoomAnimate && isViewResume(mNavZoomIn))
    {
      fadeInZoom();
    }

    if (!mNavMyPosition.isHideOnNavigation() && !mIsMyPosAnimate
        && isOverTopLimit(mNavMyPosition.getButton()))
    {
      fadeOutMyPosition();
    }
    else if (!mNavMyPosition.isHideOnNavigation() && !mIsMyPosAnimate
             && isViewResume(mNavMyPosition.getButton()))
    {
      fadeInMyPosition();
    }
  }

  private boolean isOverTopLimit(@NonNull View view)
  {
    return view.getVisibility() == View.VISIBLE && view.getY() <= mTop;
  }
  private boolean isViewResume(@NonNull View view)
  {
    return view.getVisibility() == View.INVISIBLE && view.getY() >= mTop;
  }
}
