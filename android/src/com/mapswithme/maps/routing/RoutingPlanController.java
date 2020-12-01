package com.mapswithme.maps.routing;

import android.app.Activity;
import android.os.Bundle;
import androidx.annotation.DrawableRes;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.settings.DrivingOptionsActivity;
import com.mapswithme.maps.taxi.TaxiInfo;
import com.mapswithme.maps.taxi.TaxiManager;
import com.mapswithme.maps.widget.RoutingToolbarButton;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

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
  private final WheelProgressView mProgressTaxi;

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
    CompoundButton.OnCheckedChangeListener listener = new CompoundButton.OnCheckedChangeListener()
    {
      @Override
      public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
      {
        RoutingToolbarButton button = (RoutingToolbarButton) buttonView;
        button.setIcon(iconRes);
        if (isChecked)
          button.activate();
        else
          button.deactivate();
      }
    };

    RoutingToolbarButton rb = (RoutingToolbarButton) mRouterTypes.findViewById(buttonId);
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

    mRouterTypes = (RadioGroup) getToolbar().findViewById(R.id.route_type);

    setupRouterButtons();

    View progressFrame = getToolbar().findViewById(R.id.progress_frame);
    mProgressVehicle = (WheelProgressView) progressFrame.findViewById(R.id.progress_vehicle);
    mProgressPedestrian = (WheelProgressView) progressFrame.findViewById(R.id.progress_pedestrian);
    mProgressTransit = (WheelProgressView) progressFrame.findViewById(R.id.progress_transit);
    mProgressBicycle = (WheelProgressView) progressFrame.findViewById(R.id.progress_bicycle);
    mProgressTaxi = (WheelProgressView) progressFrame.findViewById(R.id.progress_taxi);

    mRoutingBottomMenuController = RoutingBottomMenuController.newInstance(requireActivity(), mFrame, listener);

    mDrivingOptionsBtnContainer = mFrame.findViewById(R.id.driving_options_btn_container);
    View btn = mDrivingOptionsBtnContainer.findViewById(R.id.driving_options_btn);
    mDrivingOptionsImage = mFrame.findViewById(R.id.driving_options_btn_img);

    btn.setOnClickListener(v -> DrivingOptionsActivity.start(requireActivity()));
    mDriverOptionsLayoutListener = new SelfTerminatedDrivingOptionsLayoutListener();
    mAnimToggle = MwmApplication.from(activity.getApplicationContext())
                                .getResources().getInteger(R.integer.anim_default);
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
    setupRouterButton(R.id.bicycle, R.drawable.ic_bike, this::onBicycleModeSelected);
    setupRouterButton(R.id.taxi, R.drawable.ic_taxi, this::onTaxiModeSelected);
    setupRouterButton(R.id.transit, R.drawable.ic_transit, v -> onTransitModeSelected());
  }

  private void onTransitModeSelected()
  {
    AlohaHelper.logClick(AlohaHelper.ROUTING_TRANSIT_SET);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_TRANSIT_SET);
    RoutingController.get().setRouterType(Framework.ROUTER_TYPE_TRANSIT);
  }

  private void onTaxiModeSelected(@NonNull View v)
  {
    AlohaHelper.logClick(AlohaHelper.ROUTING_TAXI_SET);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_TAXI_SET);
    RoutingController.get().setRouterType(Framework.ROUTER_TYPE_TAXI);
  }

  private void onBicycleModeSelected(@NonNull View v)
  {
    AlohaHelper.logClick(AlohaHelper.ROUTING_BICYCLE_SET);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_BICYCLE_SET);
    RoutingController.get().setRouterType(Framework.ROUTER_TYPE_BICYCLE);
  }

  private void onPedestrianModeSelected(@NonNull View v)
  {
    AlohaHelper.logClick(AlohaHelper.ROUTING_PEDESTRIAN_SET);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_PEDESTRIAN_SET);
    RoutingController.get().setRouterType(Framework.ROUTER_TYPE_PEDESTRIAN);
  }

  private void onVehicleModeSelected(@NonNull View v)
  {
    AlohaHelper.logClick(AlohaHelper.ROUTING_VEHICLE_SET);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_VEHICLE_SET);
    RoutingController.get().setRouterType(Framework.ROUTER_TYPE_VEHICLE);
  }

  @Override
  public void onUpClick()
  {
    AlohaHelper.logClick(AlohaHelper.ROUTING_CANCEL);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_CANCEL);
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

    if (!isTaxiRouterType())
    {
      mRoutingBottomMenuController.setStartButton();
      mRoutingBottomMenuController.showAltitudeChartAndRoutingDetails();
    }
  }

  public void updateBuildProgress(int progress, @Framework.RouterType int router)
  {
    UiUtils.invisible(mProgressVehicle, mProgressPedestrian, mProgressTransit,
                      mProgressBicycle, mProgressTaxi);
    WheelProgressView progressView;
    if (router == Framework.ROUTER_TYPE_VEHICLE)
    {
      mRouterTypes.check(R.id.vehicle);
      progressView = mProgressVehicle;
    }
    else if (router == Framework.ROUTER_TYPE_PEDESTRIAN)
    {
      mRouterTypes.check(R.id.pedestrian);
      progressView = mProgressPedestrian;
    }
    else if (router == Framework.ROUTER_TYPE_TAXI)
    {
      mRouterTypes.check(R.id.taxi);
      progressView = mProgressTaxi;
    }
    else if (router == Framework.ROUTER_TYPE_TRANSIT)
    {
      mRouterTypes.check(R.id.transit);
      progressView = mProgressTransit;
    }
    else
    {
      mRouterTypes.check(R.id.bicycle);
      progressView = mProgressBicycle;
    }

    RoutingToolbarButton button = (RoutingToolbarButton)mRouterTypes
        .findViewById(mRouterTypes.getCheckedRadioButtonId());
    button.progress();

    updateProgressLabels();

    if (RoutingController.get().isTaxiRequestHandled())
    {
      if (!RoutingController.get().isInternetConnected())
      {
        showNoInternetError();
        return;
      }
      button.complete();
      return;
    }

    if (!RoutingController.get().isBuilding() && !RoutingController.get().isTaxiPlanning())
    {
      button.complete();
      return;
    }

    UiUtils.show(progressView);
    progressView.setPending(progress == 0);
    if (progress != 0)
      progressView.setProgress(progress);
  }

  private boolean isTaxiRouterType()
  {
    return RoutingController.get().isTaxiRouterType();
  }

  private boolean isTransitType()
  {
    return RoutingController.get().isTransitType();
  }

  public void showTaxiInfo(@NonNull TaxiInfo info)
  {
    mRoutingBottomMenuController.showTaxiInfo(info);
  }

  public void showTaxiError(@NonNull TaxiManager.ErrorCode code)
  {
    switch (code)
    {
      case NoProducts:
        showError(R.string.taxi_not_found);
        break;
      case RemoteError:
        showError(R.string.dialog_taxi_error);
        break;
      case NoProviders:
        showError(R.string.taxi_no_providers);
        break;
      default:
        throw new AssertionError("Unsupported uber error: " + code);
    }
  }

  private void showNoInternetError()
  {
    @IdRes
    int checkedId = mRouterTypes.getCheckedRadioButtonId();
    RoutingToolbarButton rb = (RoutingToolbarButton) mRouterTypes.findViewById(checkedId);
    rb.error();
    showError(R.string.dialog_taxi_offline);
  }

  private void showError(@StringRes int message)
  {
    mRoutingBottomMenuController.showError(message);
  }

  void showStartButton(boolean show)
  {
    mRoutingBottomMenuController.showStartButton(show);
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
