package com.mapswithme.maps;

import android.content.res.Resources;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationState;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.Config;
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
  private final OnTranslationChangedListener mTranslationListener;

  private final float mMargin;
  private float mContentHeight;
  private float mMyPositionBottom;

  private float mTopLimit;
  private float mBottomLimit;

  private float mCompassHeight;

  NavigationButtonsAnimationController(@NonNull View zoomIn, @NonNull View zoomOut,
                                       @NonNull View myPosition, @NonNull final View contentView,
                                       @Nullable OnTranslationChangedListener translationListener)
  {
    mZoomIn = zoomIn;
    mZoomOut = zoomOut;
    checkZoomButtonsVisibility();
    mMyPosition = myPosition;
    Resources res = mZoomIn.getResources();
    mMargin = res.getDimension(R.dimen.margin_base_plus);
    mBottomLimit = res.getDimension(R.dimen.menu_line_height);
    mCompassHeight = res.getDimension(R.dimen.compass_height);
    calculateLimitTranslations();
    contentView.addOnLayoutChangeListener(new View.OnLayoutChangeListener()
    {
      @Override
      public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft,
                                 int oldTop, int oldRight, int oldBottom)
      {
        mContentHeight = bottom - top;
        contentView.removeOnLayoutChangeListener(this);
      }
    });
    mTranslationListener = translationListener;
  }

  private void checkZoomButtonsVisibility()
  {
    UiUtils.showIf(showZoomButtons(), mZoomIn, mZoomOut);
  }


  private void calculateLimitTranslations()
  {
    mTopLimit = mMargin;
    mMyPosition.addOnLayoutChangeListener(new View.OnLayoutChangeListener()
    {
      @Override
      public void onLayoutChange(View v, int left, int top, int right, int bottom,
                                 int oldLeft, int oldTop, int oldRight, int oldBottom)
      {
        mMyPositionBottom = bottom;
        mMyPosition.removeOnLayoutChangeListener(this);
      }
    });
  }

  void setTopLimit(float limit)
  {
    mTopLimit = limit + mMargin;
    update();
  }

  void setBottomLimit(float limit)
  {
    mBottomLimit = limit;
    update();
  }

  void move(float translationY)
  {
    if (mMyPositionBottom == 0 || mContentHeight == 0)
      return;

    final float translation = translationY - mMyPositionBottom;
    update(translation <= 0 ? translation : 0);
  }

  private void update()
  {
    update(mZoomIn.getTranslationY());
  }

  private void update(final float translation)
  {
    mMyPosition.setTranslationY(translation);
    mZoomOut.setTranslationY(translation);
    mZoomIn.setTranslationY(translation);
    if (mZoomIn.getVisibility() == View.VISIBLE
        && !isViewInsideLimits(mZoomIn))
    {
      UiUtils.invisible(mZoomIn, mZoomOut);
      if (mTranslationListener != null)
        mTranslationListener.onFadeOutZoomButtons();
    }
    else if (mZoomIn.getVisibility() == View.INVISIBLE
             && isViewInsideLimits(mZoomIn))
    {
      UiUtils.show(mZoomIn, mZoomOut);
      if (mTranslationListener != null)
        mTranslationListener.onFadeInZoomButtons();
    }

    if (!shouldBeHidden() && mMyPosition.getVisibility() == View.VISIBLE
        && !isViewInsideLimits(mMyPosition))
    {
      UiUtils.invisible(mMyPosition);
    }
    else if (!shouldBeHidden() && mMyPosition.getVisibility() == View.INVISIBLE
             && isViewInsideLimits(mMyPosition))
    {
      UiUtils.show(mMyPosition);
    }
    if (mTranslationListener != null)
      mTranslationListener.onTranslationChanged(translation);
  }

  private boolean isViewInsideLimits(@NonNull View view)
  {
    return view.getY() >= mTopLimit &&
           view.getBottom() + view.getTranslationY() <= mContentHeight - mBottomLimit;
  }

  private boolean shouldBeHidden()
  {
    return LocationHelper.INSTANCE.getMyPositionMode() == LocationState.FOLLOW_AND_ROTATE
           && (RoutingController.get().isPlanning() || RoutingController.get().isNavigating());
  }

  void disappearZoomButtons()
  {
    if (!showZoomButtons())
      return;

    UiUtils.hide(mZoomIn, mZoomOut);
  }

  void appearZoomButtons()
  {
    if (!showZoomButtons())
      return;

    UiUtils.show(mZoomIn, mZoomOut);
  }

  private static boolean showZoomButtons()
  {
    return Config.showZoomButtons();
  }

  public void onResume()
  {
    checkZoomButtonsVisibility();
  }

  boolean isConflictWithCompass(int compassOffset)
  {
    int zoomTop = mZoomIn.getTop();
    return zoomTop != 0 && zoomTop <= compassOffset + mCompassHeight;
  }

  interface OnTranslationChangedListener
  {
    void onTranslationChanged(float translation);

    void onFadeInZoomButtons();

    void onFadeOutZoomButtons();
  }
}
