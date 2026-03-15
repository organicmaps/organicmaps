package app.organicmaps.routing;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.DrawableRes;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.maplayer.MapButtonsController;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.routing.TransitRouteInfo;
import app.organicmaps.sdk.util.SharedPropertiesUtils;
import app.organicmaps.settings.DrivingOptionsActivity;
import app.organicmaps.util.UiUtils;
import app.organicmaps.widget.RoutingToolbarButton;
import app.organicmaps.widget.WheelProgressView;
import com.google.android.material.bottomsheet.BottomSheetBehavior;

public class RoutingPlanController implements View.OnLayoutChangeListener
{
  private final RoutingPlanViewModel mViewModel;
  @NonNull
  private final View mRoutingDetails;
  @NonNull
  private final View mDrivingOptionsBtn;
  @NonNull
  private final View mFrame;
  @NonNull
  private final View mRoutingRoot;
  @NonNull
  private final View mRoutingSheetContainer;
  @NonNull
  private final RadioGroup mRouterTypes;
  @NonNull
  private final View mProgressFrame;
  @NonNull
  private final WheelProgressView mProgressVehicle;
  @NonNull
  private final WheelProgressView mProgressPedestrian;
  @NonNull
  private final WheelProgressView mProgressTransit;
  @NonNull
  private final WheelProgressView mProgressBicycle;
  @NonNull
  private final WheelProgressView mProgressRuler;
  @NonNull
  private final RoutingBottomMenuController mRoutingBottomMenuController;
  @NonNull
  private final TextView mDrivingOptionsBadge;
  @NonNull
  private final View mSearchBtn;
  @NonNull
  private final View mBookmarkBtn;
  @NonNull
  private final View mActionFrame;
  @NonNull
  private final View mRoutingContainer;
  @NonNull
  private final View mRoutingTypesContainer;
  private final MapButtonsController.MapButtonClickListener mMapButtonClickListener;
  @NonNull
  private final Activity mActivity;
  private BottomSheetBehavior<View> mSheetBehavior;
  private final int mBottomButtonsMaxHeight;
  private final int mPeekHeightMargins;
  @NonNull
  private final View mButtonsLayout;
  private int topInset;
  private final View closeButton;

  private final Observer<Boolean> mShowRoutingBottomSheetObserver = this::showSheet;
  private final Observer<Boolean> mIsPlacePageActiveObserver = s -> showSheet(s == null || !s);

  public RoutingPlanController(@NonNull Activity activity,
                               @NonNull ActivityResultLauncher<Intent> startDrivingOptionsForResult,
                               @NonNull RoutingBottomMenuListener listener)
  {
    mActivity = activity;
    mViewModel = new ViewModelProvider((MwmActivity) mActivity).get(RoutingPlanViewModel.class);
    mMapButtonClickListener = (MwmActivity) mActivity;
    mFrame = mActivity.findViewById(R.id.routing_sheet_frame);
    mRoutingTypesContainer = mFrame.findViewById(R.id.routing_types_frame);
    mRouterTypes = mFrame.findViewById(R.id.route_type2);
    mPeekHeightMargins = mActivity.getResources().getDimensionPixelSize(R.dimen.routing_bottom_buttons_max_height);
    mBottomButtonsMaxHeight = mActivity.getResources().getDimensionPixelSize(R.dimen.routing_margin_peek_height);

    setupRouterButtons();

    mProgressFrame = mFrame.findViewById(R.id.progress_frame2);
    mProgressVehicle = mProgressFrame.findViewById(R.id.progress_vehicle);
    mProgressPedestrian = mProgressFrame.findViewById(R.id.progress_pedestrian);
    mProgressTransit = mProgressFrame.findViewById(R.id.progress_transit);
    mProgressBicycle = mProgressFrame.findViewById(R.id.progress_bicycle);
    mProgressRuler = mProgressFrame.findViewById(R.id.progress_ruler);

    mRoutingDetails = mFrame.findViewById(R.id.routing_details);
    mActionFrame = mActivity.findViewById(R.id.routing_action_frame);
    mRoutingContainer = mActivity.findViewById(R.id.routing_container);
    mButtonsLayout = mActivity.findViewById(R.id.routing_bottom_buttons);

    mRoutingBottomMenuController = RoutingBottomMenuController.newInstance(mActivity, mFrame, listener);
    mRoutingBottomMenuController.setVisibilityChangedCallback(this::updateMapButtonsOffset);
    mRoutingRoot = mActivity.findViewById(R.id.routing_root);

    mDrivingOptionsBadge = mFrame.findViewById(R.id.driving_options_badge);
    mDrivingOptionsBtn = mRoutingDetails.findViewById(R.id.driving_options_btn_img);
    mDrivingOptionsBtn.setOnClickListener(v -> DrivingOptionsActivity.start(mActivity, startDrivingOptionsForResult));

    mSearchBtn = mRoutingRoot.findViewById(R.id.routing_btn_search);
    mBookmarkBtn = mButtonsLayout.findViewById(R.id.routing_btn_bookmarks);
    mSearchBtn.setOnClickListener(
        v -> mMapButtonClickListener.onMapButtonClick(MapButtonsController.MapButtons.search));
    mBookmarkBtn.setOnClickListener(
        v -> mMapButtonClickListener.onMapButtonClick(MapButtonsController.MapButtons.bookmarks));

    mRoutingSheetContainer = mActivity.findViewById(R.id.routing_sheet_container);
    closeButton = mRoutingTypesContainer.findViewById(R.id.back);
    closeButton.setOnClickListener(v -> mActivity.onBackPressed());

    /// TODO :
    // smooth animation to avoid snappiness
    //    mRoutingDetails.addOnLayoutChangeListener((v, left, top, right, bottom, oldLeft, oldTop, oldRight, oldBottom)
    //    -> {
    //      int oldHeight = oldBottom - oldTop;
    //      int newHeight = bottom - top;
    //      if (oldHeight > 0 && newHeight != oldHeight)
    //      {
    //        TransitionManager.beginDelayedTransition((ViewGroup) v.getParent(), new AutoTransition());
    //      }
    //    });

    setInsets();
    setupBottomSheetBehavior();
  }

  private void setInsets()
  {
    // we can't add the following complex insets through XML styles ...
    ViewCompat.setOnApplyWindowInsetsListener(mRoutingRoot, (v, insets) -> {
      final int leftInset = insets.getInsets(WindowInsetsCompat.Type.systemBars()).left;
      final int rightInset = insets.getInsets(WindowInsetsCompat.Type.systemBars()).right;
      final int bottomInset = insets.getInsets(WindowInsetsCompat.Type.systemBars()).bottom;
      topInset = insets.getInsets(WindowInsetsCompat.Type.statusBars()).top;

      mRoutingRoot.setPadding(leftInset, topInset, rightInset, 0);
      mActionFrame.setPadding(0, 0, rightInset, bottomInset);
      mButtonsLayout.setPadding(0, 0, 0, bottomInset);
      return ViewCompat.onApplyWindowInsets(v, insets);
    });
  }

  private void setupBottomSheetBehavior()
  {
    // no need to add max height to bottomsheet as the constraints are being handled by container insets
    mSheetBehavior = BottomSheetBehavior.from(mFrame);
    mSheetBehavior.setHideable(false);
    mSheetBehavior.setFitToContents(true);
    mSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    mSheetBehavior.addBottomSheetCallback(new BottomSheetBehavior.BottomSheetCallback() {
      @Override
      public void onStateChanged(@NonNull View bottomSheet, int newState)
      {
        if (mViewModel != null)
          mViewModel.setBottomSheetState(newState);
      }

      @Override
      public void onSlide(@NonNull View bottomSheet, float slideOffset)
      {
        if (mViewModel != null)
          mViewModel.setmRoutingBottomDistanceToTop(bottomSheet.getTop() + topInset);
      }
    });
  }

  public void onStart()
  {
    if (mViewModel != null)
    {
      mViewModel.getShowRoutingBottomsheet().observe((LifecycleOwner) mActivity, mShowRoutingBottomSheetObserver);
      mViewModel.getIsPlacePageactive().observe((LifecycleOwner) mActivity, mIsPlacePageActiveObserver);
    }
    mRoutingContainer.addOnLayoutChangeListener(this);
  }

  public void onStop()
  {
    if (mViewModel != null)
    {
      mViewModel.getShowRoutingBottomsheet().removeObserver(mShowRoutingBottomSheetObserver);
      mViewModel.getIsPlacePageactive().removeObserver(mIsPlacePageActiveObserver);
    }
    mRoutingContainer.removeOnLayoutChangeListener(this);
  }

  public void showSheet(boolean show)
  {
    boolean showSheet = mViewModel != null && Boolean.TRUE.equals(mViewModel.getShowRoutingBottomsheet().getValue());
    if (show && showSheet)
    {
      UiUtils.show(mButtonsLayout, mFrame);
      mFrame.post(() -> {
        mSheetBehavior.setHideable(false);
        final Integer state = mViewModel.getBottomSheetState().getValue();
        mSheetBehavior.setState(state != BottomSheetBehavior.STATE_SETTLING ? state
                                                                            : BottomSheetBehavior.STATE_COLLAPSED);
      });
    }
    else
    {
      mSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
      mSheetBehavior.setHideable(true);
      UiUtils.hide(mButtonsLayout);
    }
  }

  public void setRoutingContentActive(boolean active)
  {
    if (active)
    {
      mDrivingOptionsBtn.setEnabled(true);
      mRoutingDetails.setAlpha(1.0f);
    }
    else
    {
      {
        mDrivingOptionsBtn.setEnabled(false);
        mRoutingDetails.setAlpha(0.2f);
      }
    }
  }

  public void updateMapButtonsOffset()
  {
    // to be implemented to properly show the route avoiding the bottomsheet
    //    if (mCoordinator != null && mSheetTop != (mCoordinator.getHeight() - mFrame.getHeight()))
    //      Framework.nativeSetVisibleRect(0, 0, mFrame.getWidth(), mCoordinator.getHeight() - mFrame.getHeight());
    if (mFrame.getTop() == 0)
      return;
    final int newPeekHeight =
        mRoutingTypesContainer.getHeight() + mRoutingDetails.getHeight() + mBottomButtonsMaxHeight + mPeekHeightMargins;
    mSheetBehavior.setPeekHeight(newPeekHeight);
    final int newDistanceTop = mFrame.getTop() + topInset;
    mViewModel.setmRoutingBottomDistanceToTop(newDistanceTop);
  }

  private void setupRouterButton(@IdRes int buttonId, final @DrawableRes int iconRes,
                                 View.OnClickListener clickListener)
  {
    CompoundButton.OnCheckedChangeListener listener = (buttonView, isChecked) ->
    {
      RoutingToolbarButton button = (RoutingToolbarButton) buttonView;
      button.setIcon(iconRes);
      if (isChecked)
        button.activate();
      else
        button.deactivate();
    };

    RoutingToolbarButton rb = mRouterTypes.findViewById(buttonId);
    listener.onCheckedChanged(rb, false);
    rb.setOnCheckedChangeListener(listener);
    rb.setOnClickListener(clickListener);
  }

  private void setupRouterButtons()
  {
    setupRouterButton(R.id.vehicle2, R.drawable.ic_car, this::onVehicleModeSelected);
    setupRouterButton(R.id.pedestrian2, R.drawable.ic_pedestrian, this::onPedestrianModeSelected);
    setupRouterButton(R.id.transit2, R.drawable.ic_transit, this::onTransitModeSelected);
    setupRouterButton(R.id.bicycle2, R.drawable.ic_bike, this::onBicycleModeSelected);
    setupRouterButton(R.id.ruler2, app.organicmaps.sdk.R.drawable.ic_ruler_route, this::onRulerModeSelected);
  }

  private void onTransitModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Router.Transit);
  }

  private void onBicycleModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Router.Bicycle);
  }

  private void onRulerModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Router.Ruler);
  }

  private void onPedestrianModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Router.Pedestrian);
  }

  private void onVehicleModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Router.Vehicle);
  }

  private void updateProgressLabels()
  {
    RoutingController.BuildState buildState = RoutingController.get().getBuildState();
    final boolean ready = (buildState == RoutingController.BuildState.BUILT);
    if (!ready)
      return;

    if (isTransitType())
    {
      TransitRouteInfo info = RoutingController.get().getCachedTransitInfo();
      if (info != null)
        mRoutingBottomMenuController.showTransitInfo(info);
      return;
    }

    if (isRulerType())
    {
      RoutingInfo routingInfo = RoutingController.get().getCachedRoutingInfo();
      if (routingInfo != null)
        mRoutingBottomMenuController.showRulerInfo(Framework.nativeGetRoutePoints(), routingInfo.distToTarget);
      return;
    }

    final boolean showStartButton = !RoutingController.get().isRulerRouterType();
    mRoutingBottomMenuController.setStartButton(showStartButton);
    mRoutingBottomMenuController.showAltitudeChartAndRoutingDetails();
  }

  public void updateBadgeCount(int count)
  {
    if (count > 0)
    {
      UiUtils.show(mDrivingOptionsBadge);
      mDrivingOptionsBadge.setText(String.valueOf(count));
    }
    else
    {
      UiUtils.hide(mDrivingOptionsBadge);
    }
  }

  public void updateBuildProgress(int progress, @NonNull Router router)
  {
    UiUtils.invisible(mProgressVehicle, mProgressPedestrian, mProgressTransit, mProgressBicycle, mProgressRuler);
    WheelProgressView progressView;
    switch (router)
    {
    case Vehicle:
      mRouterTypes.check(R.id.vehicle2);
      progressView = mProgressVehicle;
      break;
    case Pedestrian:
      mRouterTypes.check(R.id.pedestrian2);
      progressView = mProgressPedestrian;
      break;
    case Transit:
      mRouterTypes.check(R.id.transit2);
      progressView = mProgressTransit;
      break;
    case Bicycle:
      mRouterTypes.check(R.id.bicycle2);
      progressView = mProgressBicycle;
      break;
    case Ruler:
      mRouterTypes.check(R.id.ruler2);
      progressView = mProgressRuler;
      break;
    default: throw new IllegalArgumentException("unknown router: " + router);
    }

    int bt = mRouterTypes.getCheckedRadioButtonId();
    RoutingToolbarButton button = mRouterTypes.findViewById(bt);
    if (button == null)
      return;

    button.progress();
    updateProgressLabels();

    if (!RoutingController.get().isBuilding())
    {
      button.complete();
      return;
    }

    UiUtils.show(progressView);
    progressView.setPending(progress == 0);
    if (progress != 0)
      progressView.setProgress(progress);
  }

  private boolean isTransitType()
  {
    return RoutingController.get().isTransitType();
  }

  private boolean isRulerType()
  {
    return RoutingController.get().isRulerRouterType();
  }

  public void saveRoutingPanelState(@NonNull Bundle outState)
  {
    mRoutingBottomMenuController.saveRoutingPanelState(outState);
  }

  public void restoreRoutingPanelState(@NonNull Bundle state)
  {
    mRoutingBottomMenuController.restoreRoutingPanelState(state);
    updateBadgeCount(SharedPropertiesUtils.getDrivingOptionsCount());
  }

  public void showAddStartFrame()
  {
    mRoutingBottomMenuController.showAddStartFrame();
  }

  public void showAddFinishFrame()
  {
    mRoutingBottomMenuController.showAddFinishFrame();
  }

  public void hideActionFrame()
  {
    mRoutingBottomMenuController.hideActionFrame();
  }

  @Override
  public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight,
                             int oldBottom)
  {
    updateMapButtonsOffset();
  }
}
