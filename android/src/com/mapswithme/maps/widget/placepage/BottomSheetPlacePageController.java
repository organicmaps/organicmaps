package com.mapswithme.maps.widget.placepage;

import android.animation.Animator;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.graphics.Rect;
import android.location.Location;
import android.support.annotation.NonNull;
import android.support.design.widget.BottomSheetBehavior;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationListener;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.PlacePageTracker;

public class BottomSheetPlacePageController implements PlacePageController, LocationListener,
                                                       View.OnLayoutChangeListener
{
  private static final int BUTTONS_ANIMATION_DURATION = 100;
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = BottomSheetPlacePageController.class.getSimpleName();
  @NonNull
  private final Activity mActivity;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private BottomSheetBehavior<View> mPlacePageBehavior;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mButtonsLayout;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PlacePageView mPlacePage;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PlacePageTracker mPlacePageTracker;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private Toolbar mToolbar;
  private int mLastPeekHeight;
  private int mViewportMinHeight;
  @NonNull
  private final BottomSheetBehavior.BottomSheetCallback mSheetCallback
      = new BottomSheetBehavior.BottomSheetCallback()

  {
    @Override
    public void onStateChanged(@NonNull View bottomSheet, int newState)
    {
      LOGGER.d(TAG, "State change, new = " + BottomSheetPlacePageController.toString(newState)
                    + " placepage height = " + mPlacePage.getHeight());
      if (newState == BottomSheetBehavior.STATE_SETTLING
          || newState == BottomSheetBehavior.STATE_DRAGGING)
        return;

      if (newState == BottomSheetBehavior.STATE_HIDDEN)
      {
        hideButtons();
        return;
      }

      showButtons();
    }

    @Override
    public void onSlide(@NonNull View bottomSheet, float slideOffset)
    {
      updateViewPortRect();
    }
  };

  public BottomSheetPlacePageController(@NonNull Activity activity)
  {
    mActivity = activity;
  }

  @Override
  public void initialize()
  {
    mViewportMinHeight = mActivity.getResources().getDimensionPixelSize(R.dimen.viewport_min_height);
    mToolbar = mActivity.findViewById(R.id.pp_toolbar);
    UiUtils.extendViewWithStatusBar(mToolbar);
    UiUtils.showHomeUpButton(mToolbar);
    mToolbar.setNavigationOnClickListener(v -> close());
    mPlacePage = mActivity.findViewById(R.id.placepage);
    mPlacePageBehavior = BottomSheetBehavior.from(mPlacePage);
    mPlacePageBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    mPlacePageBehavior.setBottomSheetCallback(mSheetCallback);
    mPlacePage.addOnLayoutChangeListener(this);
    mButtonsLayout = mActivity.findViewById(R.id.pp_buttons_layout);
    ViewGroup buttons = mButtonsLayout.findViewById(R.id.container);
    mPlacePage.initButtons(buttons);
    UiUtils.bringViewToFrontOf(mButtonsLayout, mPlacePage);
    UiUtils.bringViewToFrontOf(mActivity.findViewById(R.id.app_bar), mPlacePage);
    mPlacePageTracker = new PlacePageTracker(mPlacePage, buttons);
    LocationHelper.INSTANCE.addListener(this);
  }

  @Override
  public void destroy()
  {
    LocationHelper.INSTANCE.removeListener(this);
  }

  @Override
  public void openFor(@NonNull MapObject object)
  {
    mPlacePage.setMapObject(object, false, () -> {
      if (object.isExtendedView())
      {
        mPlacePageBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
        return;
      }

      openBottomSheet();
    });
    mToolbar.setTitle(object.getTitle());
    mPlacePageTracker.setMapObject(object);
    Framework.logLocalAdsEvent(Framework.LocalAdsEventType.LOCAL_ADS_EVENT_OPEN_INFO, object);
  }

  private void openBottomSheet()
  {
    mPlacePage.post(() -> {
      int peekHeight = getPeekHeight();
      LOGGER.d(TAG, "Peek height = " + peekHeight);
      mLastPeekHeight = peekHeight;
      mPlacePageBehavior.setPeekHeight(mLastPeekHeight);
      mPlacePageBehavior.setState(BottomSheetBehavior.STATE_COLLAPSED);
    });
  }

  private int getPeekHeight()
  {
    return mPlacePage.getPreviewHeight() + mButtonsLayout.getHeight();
  }

  @Override
  public void close()
  {
    mPlacePageBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    mPlacePage.hide();
  }

  private void showButtons()
  {
    ObjectAnimator animator = ObjectAnimator.ofFloat(mButtonsLayout, "translationY",
                                                     0);
    animator.setDuration(BUTTONS_ANIMATION_DURATION);
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationStart(Animator animation)
      {
        UiUtils.show(mButtonsLayout);
        super.onAnimationStart(animation);
      }
    });
    animator.start();
  }

  private void hideButtons()
  {
    ObjectAnimator animator = ObjectAnimator.ofFloat(mButtonsLayout, "translationY",
                                                     mButtonsLayout.getMeasuredHeight());
    animator.setDuration(BUTTONS_ANIMATION_DURATION);
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        super.onAnimationEnd(animation);
        UiUtils.invisible(mButtonsLayout);
      }
    });
    animator.start();
  }

  @Override
  public boolean isClosed()
  {
    return mPlacePageBehavior.getState() == BottomSheetBehavior.STATE_HIDDEN;
  }

  @Override
  public void onLocationUpdated(Location location)
  {
    mPlacePage.refreshLocation(location);
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    double north = trueNorth >= 0.0 ? trueNorth : magneticNorth;
    mPlacePage.refreshAzimuth(north);
  }

  @Override
  public void onLocationError(int errorCode)
  {
    // Do nothing by default.
  }

  @Override
  public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int
      oldTop, int oldRight, int oldBottom)
  {
    LOGGER.d(TAG, "Layout changed, current state  = " + toString(mPlacePageBehavior.getState()));
    if (mPlacePageBehavior.getState() != BottomSheetBehavior.STATE_COLLAPSED)
      return;

    if (getPeekHeight() == mLastPeekHeight)
      return;

    openBottomSheet();
  }

  private void updateViewPortRect()
  {
    View coordinatorLayout = (ViewGroup) mPlacePage.getParent();
    int viewPortWidth = coordinatorLayout.getWidth();
    int viewPortHeight = coordinatorLayout.getHeight();
    Rect sheetRect = new Rect();
    mPlacePage.getGlobalVisibleRect(sheetRect);
    if (sheetRect.top < mViewportMinHeight)
      return;

    if (sheetRect.top >= viewPortHeight)
    {
      Framework.nativeSetVisibleRect(0, 0, viewPortWidth, viewPortHeight);
      return;
    }
    viewPortHeight -= sheetRect.height();
    LOGGER.d(TAG, "Viewport room: 0, 0, " + viewPortWidth + ", " + viewPortHeight);
    Framework.nativeSetVisibleRect(0, 0, viewPortWidth, viewPortHeight);
  }

  @NonNull
  private static String toString(@BottomSheetBehavior.State int state)
  {
    switch (state)
    {
      case BottomSheetBehavior.STATE_EXPANDED:
        return "EXPANDED";
      case BottomSheetBehavior.STATE_COLLAPSED:
        return "COLLAPSED";
      case BottomSheetBehavior.STATE_DRAGGING:
        return "DRAGGING";
      case BottomSheetBehavior.STATE_SETTLING:
        return "SETTLING";
      case BottomSheetBehavior.STATE_HIDDEN:
        return "HIDDEN";
      default:
        throw new AssertionError("Unsupported state detected: " + state);
    }
  }
}
