package com.mapswithme.maps.routing;

import android.app.Activity;
import android.view.View;
import android.widget.ImageView;
import android.widget.RadioGroup;
import android.widget.TextView;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.AlohaHelper;

public class RoutingPlanController extends ToolbarController
{
  protected final View mFrame;
  private final ImageView mToggle;
  private final SlotFrame mSlotFrame;
  private final RadioGroup mRouterTypes;
  private final WheelProgressView mProgressVehicle;
  private final WheelProgressView mProgressPedestrian;
  private final View mPlanningLabel;
  private final View mErrorLabel;
  private final View mNumbersFrame;
  private final TextView mNumbersTime;
  private final TextView mNumbersDistance;

  public RoutingPlanController(View root, Activity activity)
  {
    super(root, activity);
    mFrame = root;

    mToggle = (ImageView) mToolbar.findViewById(R.id.toggle);
    mSlotFrame = new SlotFrame(root);

    View planFrame = root.findViewById(R.id.planning_frame);

    mRouterTypes = (RadioGroup) planFrame.findViewById(R.id.route_type);
    mRouterTypes.findViewById(R.id.vehicle).setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        AlohaHelper.logClick(AlohaHelper.ROUTING_VEHICLE_SET);
        RoutingController.get().setRouterType(Framework.ROUTER_TYPE_VEHICLE);
      }
    });

    mRouterTypes.findViewById(R.id.pedestrian).setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        AlohaHelper.logClick(AlohaHelper.ROUTING_PEDESTRIAN_SET);
        RoutingController.get().setRouterType(Framework.ROUTER_TYPE_PEDESTRIAN);
      }
    });

    View progressFrame = planFrame.findViewById(R.id.progress_frame);
    mProgressVehicle = (WheelProgressView) progressFrame.findViewById(R.id.progress_vehicle);
    mProgressPedestrian = (WheelProgressView) progressFrame.findViewById(R.id.progress_pedestrian);

    mPlanningLabel = planFrame.findViewById(R.id.planning);
    mErrorLabel = planFrame.findViewById(R.id.error);
    mNumbersFrame = planFrame.findViewById(R.id.numbers);
    mNumbersTime = (TextView) mNumbersFrame.findViewById(R.id.time);
    mNumbersDistance = (TextView) mNumbersFrame.findViewById(R.id.distance);

    setTitle(R.string.route);

    mToggle.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        toggleSlots();
      }
    });
  }

  @Override
  protected void onUpClick()
  {
    AlohaHelper.logClick(AlohaHelper.ROUTING_GO_CLOSE);
    RoutingController.get().cancel();
  }

  private void updateToggle()
  {
    if (mToggle.getVisibility() != View.VISIBLE)
      return;


  }

  public void updatePoints()
  {
    mSlotFrame.update();
  }

  private void updateProgressLabels()
  {
    UiUtils.showIf(RoutingController.get().isBuilding(), mPlanningLabel);

    RoutingController.BuildState buildState = RoutingController.get().getBuildState();
    UiUtils.showIf(buildState == RoutingController.BuildState.ERROR, mErrorLabel);

    boolean ready = (buildState == RoutingController.BuildState.BUILT);
    UiUtils.showIf(ready, mNumbersFrame);
    if (!ready)
      return;

    RoutingInfo rinfo = Framework.nativeGetRouteFollowingInfo();
    mNumbersTime.setText(RoutingController.formatRoutingTime(rinfo.totalTimeInSeconds));
    mNumbersDistance.setText(Utils.formatUnitsText(R.dimen.text_size_routing_number, R.dimen.text_size_routing_dimension,
                                                   rinfo.distToTarget, rinfo.targetUnits));
  }

  public void updateBuildProgress(int progress, int router)
  {
    updateProgressLabels();

    boolean vehicle = (router == Framework.ROUTER_TYPE_VEHICLE);
    mRouterTypes.check(vehicle ? R.id.vehicle : R.id.pedestrian);

    if (!RoutingController.get().isBuilding())
    {
      UiUtils.hide(mProgressVehicle, mProgressPedestrian);
      return;
    }

    UiUtils.visibleIf(vehicle, mProgressVehicle);
    UiUtils.visibleIf(!vehicle, mProgressPedestrian);

    if (vehicle)
      mProgressVehicle.setProgress(progress);
    else
      mProgressPedestrian.setProgress(progress);
  }

  private void toggleSlots()
  {
    // TODO
    updateToggle();
  }

  private void showSlots(boolean show, boolean animate)
  {
    // TODO: Animation
    UiUtils.showIf(show, mSlotFrame.getFrame());
    updateToggle();
  }

  public void disableToggle()
  {
    UiUtils.hide(mToggle);
    showSlots(false, false);
  }
}
