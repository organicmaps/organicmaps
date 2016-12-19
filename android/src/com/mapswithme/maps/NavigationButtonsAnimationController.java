package com.mapswithme.maps;

import android.support.annotation.NonNull;
import android.view.View;

import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.Animations;
import com.mapswithme.util.Config;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.log.DebugLogger;

class NavigationButtonsAnimationController
{
  private static final DebugLogger LOGGER =
      new DebugLogger(NavigationButtonsAnimationController.class.getSimpleName());
  @NonNull
  private final View mZoomIn;
  @NonNull
  private final View mZoomOut;
  @NonNull
  private final View mMyPosition;

  private float mBottomLimit;
  private float mTopLimit;

  private boolean mMyPosAnimate;

  NavigationButtonsAnimationController(@NonNull View zoomIn, @NonNull View zoomOut,
                                       @NonNull View myPosition)
  {
    mZoomIn = zoomIn;
    mZoomOut = zoomOut;
    UiUtils.showIf(showZoomButtons(), mZoomIn, mZoomOut);
    mMyPosition = myPosition;
    calculateBottomLimit();
  }

  void disappearZoomButtons()
  {
    if (!showZoomButtons())
      return;

    Animations.disappearSliding(mZoomIn, Animations.RIGHT, null);
    Animations.disappearSliding(mZoomOut, Animations.RIGHT, null);
  }

  void appearZoomButtons()
  {
    if (!showZoomButtons())
      return;

    if (!canZoomButtonsFitInScreen())
      return;

    Animations.appearSliding(mZoomIn, Animations.RIGHT, null);
    Animations.appearSliding(mZoomOut, Animations.RIGHT, null);
    updateZoomButtonsPosition();
  }

  void setTopLimit(float limit)
  {
    mTopLimit = limit;
    updateZoomButtonsPosition();
  }

  void onPlacePageVisibilityChanged(boolean isVisible)
  {
    if (isVisible)
      fadeOutZooms();
    else
      fadeInZooms();
  }

  void onPlacePageMoved(float translationY)
  {
    if (mBottomLimit == 0)
      return;

    float translation = translationY - mBottomLimit;
    animateMyPosition(translation <= 0 ? translation : 0);
  }

  private void calculateBottomLimit()
  {
    mMyPosition.addOnLayoutChangeListener(new View.OnLayoutChangeListener()
    {
      @Override
      public void onLayoutChange(View v, int left, int top, int right, int bottom,
                                 int oldLeft, int oldTop, int oldRight, int oldBottom)
      {
        mBottomLimit = bottom;
        mMyPosition.removeOnLayoutChangeListener(this);
      }
    });
  }

  private void updateZoomButtonsPosition()
  {
    // It means that the zoom buttons fit in screen perfectly,
    // any updates of position are no needed.
    if (mZoomIn.getTop() >= mTopLimit && UiUtils.isVisible(mZoomIn))
      return;

    // If the top limit is decreased we try to return zoom buttons at initial position.
    if (mTopLimit < mZoomIn.getTop() && tryPlaceZoomButtonsAtInitialPosition())
    {
      LOGGER.d("Zoom buttons were come back to initial position");
      return;
    }

    // If top view overlaps the zoomIn button.
    if (mTopLimit > mZoomIn.getTop())
    {
      // We should try to pull out the zoomIn button from under the top view
      // if available space allows us doing that.
      if (tryPlaceZoomButtonsUnderTopLimit())
        return;

      // Otherwise, we just fade out the zoom buttons
      // since there is not enough space on the screen for them.
      fadeOutZooms();
    }
  }

  private boolean tryPlaceZoomButtonsAtInitialPosition()
  {
    float availableSpace = mBottomLimit - mTopLimit;
    float requiredSpace = mBottomLimit - mZoomIn.getTop() - mZoomIn.getTranslationY();
    if (requiredSpace > availableSpace)
      return false;

    mZoomIn.setTranslationY(0);
    mZoomOut.setTranslationY(0);
    return true;
  }

  private boolean tryPlaceZoomButtonsUnderTopLimit()
  {
    if (!canZoomButtonsFitInScreen())
      return false;

    float requiredTranslate = mTopLimit - mZoomIn.getTop();
    mZoomIn.setTranslationY(requiredTranslate);
    mZoomOut.setTranslationY(requiredTranslate);
    return true;
  }

  private boolean canZoomButtonsFitInScreen()
  {
    float availableSpace = mBottomLimit - mTopLimit;
    return getMinimumRequiredHeightForButtons() <= availableSpace;
  }

  private float getMinimumRequiredHeightForButtons()
  {
    return mZoomIn.getHeight() + mZoomOut.getHeight() + mMyPosition.getHeight();
  }

  private void fadeOutZooms()
  {
    Animations.fadeOutView(mZoomIn, new Runnable()
    {
      @Override
      public void run()
      {
        mZoomIn.setVisibility(View.INVISIBLE);
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

  private void fadeInZooms()
  {
    if (!showZoomButtons())
      return;

    if (!canZoomButtonsFitInScreen())
      return;

    mZoomIn.setVisibility(View.VISIBLE);
    mZoomOut.setVisibility(View.VISIBLE);
    Animations.fadeInView(mZoomIn, null);
    Animations.fadeInView(mZoomOut, null);
  }

  private void fadeOutMyPosition()
  {
    mMyPosAnimate = true;
    Animations.fadeOutView(mMyPosition, new Runnable()
    {
      @Override
      public void run()
      {
        UiUtils.invisible(mMyPosition);
        mMyPosAnimate = false;
      }
    });
  }

  private void fadeInMyPosition()
  {
    mMyPosAnimate = true;
    mMyPosition.setVisibility(View.VISIBLE);
    Animations.fadeInView(mMyPosition, new Runnable()
    {
      @Override
      public void run()
      {
        mMyPosAnimate = false;
      }
    });
  }

  private void animateMyPosition(float translation)
  {
    mMyPosition.setTranslationY(translation);
    if (!shouldMyPositionBeHidden() && !mMyPosAnimate
        && isOverTopLimit(mMyPosition))
    {
      fadeOutMyPosition();
    }
    else if (!shouldMyPositionBeHidden() && !mMyPosAnimate
             && satisfyTopLimit(mMyPosition))
    {
      fadeInMyPosition();
    }
  }

  private boolean isOverTopLimit(@NonNull View view)
  {
    return view.getVisibility() == View.VISIBLE && view.getY() <= mTopLimit;
  }

  private boolean satisfyTopLimit(@NonNull View view)
  {
    return view.getVisibility() == View.INVISIBLE && view.getY() >= mTopLimit;
  }

  private boolean shouldMyPositionBeHidden()
  {
    return LocationState.getMode() == LocationState.FOLLOW_AND_ROTATE
           && (RoutingController.get().isPlanning());
  }

  private static boolean showZoomButtons()
  {
    return Config.showZoomButtons();
  }
}
