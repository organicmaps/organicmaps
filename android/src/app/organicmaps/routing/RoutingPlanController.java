package app.organicmaps.routing;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import androidx.core.view.ViewCompat;
import app.organicmaps.Framework;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.settings.DrivingOptionsActivity;
import app.organicmaps.widget.RoutingToolbarButton;
import app.organicmaps.widget.ToolbarController;
import app.organicmaps.widget.WheelProgressView;
import app.organicmaps.util.UiUtils;

public class RoutingPlanController extends ToolbarController
{
  private static final String BUNDLE_HAS_DRIVING_OPTIONS_VIEW = "has_driving_options_view";

  private final View mFrame;
  @NonNull
  private final RoutingPlanInplaceController.RoutingPlanListener mRoutingPlanListener;
  private final RadioGroup mRouterTypes;
  @NonNull
  private final WheelProgressView mProgressVehicle;
  @NonNull
  private final WheelProgressView mProgressPedestrian;
  @NonNull
  private final WheelProgressView mProgressTransit;
  @NonNull
  private final WheelProgressView mProgressBicycle;

  @NonNull
  private final WheelProgressView mProgressHelicopter;

//  @NonNull
//  private final WheelProgressView mProgressTaxi;

  @NonNull
  private final RoutingBottomMenuController mRoutingBottomMenuController;

  int mFrameHeight;
  final int mAnimToggle;

  @NonNull
  private final View mDrivingOptionsBtnContainer;

  @NonNull
  private final View.OnLayoutChangeListener mDriverOptionsLayoutListener;

  @NonNull
  private final View mDrivingOptionsImage;

  private RadioButton setupRouterButton(@IdRes int buttonId, final @DrawableRes int iconRes, View.OnClickListener clickListener)
  {
    CompoundButton.OnCheckedChangeListener listener = (buttonView, isChecked) -> {
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
    return rb;
  }

  RoutingPlanController(View root, Activity activity,
                        @NonNull RoutingPlanInplaceController.RoutingPlanListener routingPlanListener,
                        @Nullable RoutingBottomMenuListener listener)
  {
    super(root, activity);
    mFrame = root;
    mRoutingPlanListener = routingPlanListener;

    mRouterTypes = getToolbar().findViewById(R.id.route_type);

    setupRouterButtons();

    View progressFrame = getToolbar().findViewById(R.id.progress_frame);
    mProgressVehicle = progressFrame.findViewById(R.id.progress_vehicle);
    mProgressPedestrian = progressFrame.findViewById(R.id.progress_pedestrian);
    mProgressTransit = progressFrame.findViewById(R.id.progress_transit);
    mProgressBicycle = progressFrame.findViewById(R.id.progress_bicycle);
    mProgressHelicopter = progressFrame.findViewById(R.id.progress_helicopter);
//    mProgressTaxi = (WheelProgressView) progressFrame.findViewById(R.id.progress_taxi);

    mRoutingBottomMenuController = RoutingBottomMenuController.newInstance(requireActivity(), mFrame, listener);

    mDrivingOptionsBtnContainer = mFrame.findViewById(R.id.driving_options_btn_container);
    View btn = mDrivingOptionsBtnContainer.findViewById(R.id.driving_options_btn);
    mDrivingOptionsImage = mFrame.findViewById(R.id.driving_options_btn_img);

    btn.setOnClickListener(v -> DrivingOptionsActivity.start(requireActivity()));
    mDriverOptionsLayoutListener = new SelfTerminatedDrivingOptionsLayoutListener();
    mAnimToggle = MwmApplication.from(activity.getApplicationContext())
                                .getResources().getInteger(R.integer.anim_default);

    ViewCompat.setOnApplyWindowInsetsListener(mFrame, (view, windowInsets) -> {
      UiUtils.setViewInsetsPaddingNoTop(activity.findViewById(R.id.menu_frame), windowInsets);
      return windowInsets;
    });
  }

  @NonNull
  protected View getFrame()
  {
    return mFrame;
  }

  @NonNull
  private View getDrivingOptionsBtnContainer()
  {
    return mDrivingOptionsBtnContainer;
  }

  private void setupRouterButtons()
  {
    setupRouterButton(R.id.vehicle, R.drawable.ic_car, this::onVehicleModeSelected);
    setupRouterButton(R.id.pedestrian, R.drawable.ic_pedestrian, this::onPedestrianModeSelected);
//    setupRouterButton(R.id.taxi, R.drawable.ic_taxi, this::onTaxiModeSelected);
    setupRouterButton(R.id.transit, R.drawable.ic_transit, this::onTransitModeSelected);
    setupRouterButton(R.id.bicycle, R.drawable.ic_bike, this::onBicycleModeSelected);
    setupRouterButton(R.id.helicopter, R.drawable.ic_helicopter, this::onHelicopterModeSelected);
  }

  private void onTransitModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Framework.ROUTER_TYPE_TRANSIT);
  }

  private void onBicycleModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Framework.ROUTER_TYPE_BICYCLE);
  }

  private void onHelicopterModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Framework.ROUTER_TYPE_HELICOPTER);
  }

  private void onPedestrianModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Framework.ROUTER_TYPE_PEDESTRIAN);
  }

  private void onVehicleModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Framework.ROUTER_TYPE_VEHICLE);
  }

  @Override
  public void onUpClick()
  {
    RoutingController.get().cancel();
  }

  boolean checkFrameHeight()
  {
    if (mFrameHeight > 0)
      return true;

    mFrameHeight = mFrame.getHeight();
    return (mFrameHeight > 0);
  }

  private void updateProgressLabels()
  {
    RoutingController.BuildState buildState = RoutingController.get().getBuildState();

    boolean ready = (buildState == RoutingController.BuildState.BUILT);

    if (!ready) 
    {
      mRoutingBottomMenuController.hideAltitudeChartAndRoutingDetails();
      return;
    }

    if (isTransitType())
    {
      TransitRouteInfo info = RoutingController.get().getCachedTransitInfo();
      if (info != null)
        mRoutingBottomMenuController.showTransitInfo(info);
      return;
    }

    boolean showStartButton = !RoutingController.get().isHelicopterRouterType();
    mRoutingBottomMenuController.setStartButton(showStartButton);
    mRoutingBottomMenuController.showAltitudeChartAndRoutingDetails();
  }

  public void updateBuildProgress(int progress, @Framework.RouterType int router)
  {
    UiUtils.invisible(mProgressVehicle, mProgressPedestrian, mProgressTransit,
                      mProgressBicycle, mProgressHelicopter);
    WheelProgressView progressView;
    switch(router)
    {
    case Framework.ROUTER_TYPE_VEHICLE:
      mRouterTypes.check(R.id.vehicle);
      progressView = mProgressVehicle;
      break;
    case Framework.ROUTER_TYPE_PEDESTRIAN:
      mRouterTypes.check(R.id.pedestrian);
      progressView = mProgressPedestrian;
      break;
    //case Framework.ROUTER_TYPE_TAXI:
    //    {
    //      mRouterTypes.check(R.id.taxi);
    //      progressView = mProgressTaxi;
    //    }
    case Framework.ROUTER_TYPE_TRANSIT:
      mRouterTypes.check(R.id.transit);
      progressView = mProgressTransit;
      break;
    case Framework.ROUTER_TYPE_BICYCLE:
      mRouterTypes.check(R.id.bicycle);
      progressView = mProgressBicycle;
      break;
    case Framework.ROUTER_TYPE_HELICOPTER:
      mRouterTypes.check(R.id.helicopter);
      progressView = mProgressHelicopter;
      break;
    default:
        throw new IllegalArgumentException("unknown router: "+router);
    }

    RoutingToolbarButton button = mRouterTypes
        .findViewById(mRouterTypes.getCheckedRadioButtonId());
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

  void saveRoutingPanelState(@NonNull Bundle outState)
  {
    mRoutingBottomMenuController.saveRoutingPanelState(outState);
    outState.putBoolean(BUNDLE_HAS_DRIVING_OPTIONS_VIEW, UiUtils.isVisible(mDrivingOptionsBtnContainer));
  }

  void restoreRoutingPanelState(@NonNull Bundle state)
  {
    mRoutingBottomMenuController.restoreRoutingPanelState(state);
    boolean hasView = state.getBoolean(BUNDLE_HAS_DRIVING_OPTIONS_VIEW);
    if (hasView)
      showDrivingOptionView();
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

  public void showDrivingOptionView()
  {
    mDrivingOptionsBtnContainer.addOnLayoutChangeListener(mDriverOptionsLayoutListener);
    UiUtils.show(mDrivingOptionsBtnContainer);
    boolean hasAnyOptions = RoutingOptions.hasAnyOptions();
    UiUtils.showIf(hasAnyOptions, mDrivingOptionsImage);
    TextView title = mDrivingOptionsBtnContainer.findViewById(R.id.driving_options_btn_title);
    title.setText(hasAnyOptions ? R.string.change_driving_options_btn
                                : R.string.define_to_avoid_btn);
  }

  public void hideDrivingOptionsView()
  {
    UiUtils.hide(mDrivingOptionsBtnContainer);
    mRoutingPlanListener.onRoutingPlanStartAnimate(UiUtils.isVisible(getFrame()));
  }

  public int calcHeight()
  {
    int frameHeight = getFrame().getHeight();
    if (frameHeight == 0)
      return 0;

    View driverOptionsView = getDrivingOptionsBtnContainer();
    int extraOppositeOffset = UiUtils.isVisible(driverOptionsView)
                              ? 0
                              : driverOptionsView.getHeight();

    return frameHeight - extraOppositeOffset;
  }

  @Override
  protected boolean useExtendedToolbar()
  {
    return true;
  }

  private class SelfTerminatedDrivingOptionsLayoutListener implements View.OnLayoutChangeListener
  {
    @Override
    public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft,
                               int oldTop, int oldRight, int oldBottom)
    {
      mRoutingPlanListener.onRoutingPlanStartAnimate(UiUtils.isVisible(getFrame()));
      mDrivingOptionsBtnContainer.removeOnLayoutChangeListener(this);
    }
  }
}
