package com.mapswithme.maps.widget.placepage;

import android.animation.Animator;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.location.Location;
import android.support.annotation.NonNull;
import android.support.design.widget.BottomSheetBehavior;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationListener;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.PlacePageTracker;

public class BottomSheetPlacePageController implements PlacePageController, LocationListener
{
  private static final int BUTTONS_ANIMATION_DURATION = 100;
  @NonNull
  private final Activity mActivity;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private BottomSheetBehavior<View> mPpSheetBehavior;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mButtonsLayout;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PlacePageView mPlacePage;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private ViewGroup mButtons;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PlacePageTracker mPlacePageTracker;
  @NonNull
  private final BottomSheetBehavior.BottomSheetCallback mSheetCallback
      = new BottomSheetBehavior.BottomSheetCallback()

  {
    @Override
    public void onStateChanged(@NonNull View bottomSheet, int newState)
    {
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
      // TODO: coming soon.
    }
  };

  public BottomSheetPlacePageController(@NonNull Activity activity)
  {
    mActivity = activity;
  }

  @Override
  public void initialize()
  {
    View ppSheet = mActivity.findViewById(R.id.pp_bottom_sheet);
    mPpSheetBehavior = BottomSheetBehavior.from(ppSheet);
    mPpSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    mPpSheetBehavior.setBottomSheetCallback(mSheetCallback);
    mPlacePage = ppSheet.findViewById(R.id.placepage);
    mButtonsLayout = mActivity.findViewById(R.id.pp_buttons_layout);
    mButtons = mButtonsLayout.findViewById(R.id.pp_buttons);
    mPlacePage.initButtons(mButtons.findViewById(R.id.container));
    mPlacePageTracker = new PlacePageTracker(mPlacePage, mButtons);
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
    mPlacePage.setMapObject(object, true, () -> {
      mPpSheetBehavior.setState(object.isExtendedView() ? BottomSheetBehavior.STATE_EXPANDED
                                                   : BottomSheetBehavior.STATE_COLLAPSED);
      Framework.logLocalAdsEvent(Framework.LocalAdsEventType.LOCAL_ADS_EVENT_OPEN_INFO, object);
    });
    mPlacePageTracker.setMapObject(object);
  }

  @Override
  public void close()
  {
    mPpSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
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
    return mPpSheetBehavior.getState() == BottomSheetBehavior.STATE_HIDDEN;
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
}
