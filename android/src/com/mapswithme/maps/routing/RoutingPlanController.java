package com.mapswithme.maps.routing;

import android.animation.ValueAnimator;
import android.app.Activity;
import android.os.Build;
import android.support.annotation.DrawableRes;
import android.support.annotation.IdRes;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.RotateDrawable;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

public class RoutingPlanController extends ToolbarController
{
  static final int ANIM_TOGGLE = MwmApplication.get().getResources().getInteger(R.integer.anim_slots_toggle);

  protected final View mFrame;
  private final ImageView mToggle;
  private final SlotFrame mSlotFrame;
  private final RadioGroup mRouterTypes;
  private final WheelProgressView mProgressVehicle;
  private final WheelProgressView mProgressPedestrian;
  private final View mPlanningLabel;
  private final View mErrorLabel;
  private final View mDetailsFrame;
  private final View mNumbersFrame;
  private final TextView mNumbersTime;
  private final TextView mNumbersDistance;
  private final TextView mNumbersArrival;

  private final RotateDrawable mToggleImage = new RotateDrawable(R.drawable.ic_down);
  private int mFrameHeight;
  private int mToolbarHeight;
  private boolean mOpen;

  private RadioButton setupRouterButton(@IdRes int buttonId, final @DrawableRes int iconRes, View.OnClickListener clickListener)
  {
    CompoundButton.OnCheckedChangeListener listener = new CompoundButton.OnCheckedChangeListener()
    {
      @Override
      public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
      {
        buttonView.setButtonDrawable(Graphics.tint(mActivity, iconRes, isChecked ? R.attr.colorAccent
                                                                                 : R.attr.iconTint));
      }
    };

    RadioButton rb = (RadioButton)mRouterTypes.findViewById(buttonId);
    listener.onCheckedChanged(rb, false);
    rb.setOnCheckedChangeListener(listener);
    rb.setOnClickListener(clickListener);

    return rb;
  }

  public RoutingPlanController(View root, Activity activity)
  {
    super(root, activity);
    mFrame = root;

    mToggle = (ImageView) mToolbar.findViewById(R.id.toggle);
    mSlotFrame = (SlotFrame) root.findViewById(R.id.slots);

    View planFrame = root.findViewById(R.id.planning_frame);

    mRouterTypes = (RadioGroup) planFrame.findViewById(R.id.route_type);

    setupRouterButton(R.id.vehicle, R.drawable.ic_drive, new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        AlohaHelper.logClick(AlohaHelper.ROUTING_VEHICLE_SET);
        Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_VEHICLE_SET);
        RoutingController.get().setRouterType(Framework.ROUTER_TYPE_VEHICLE);
      }
    });

    setupRouterButton(R.id.pedestrian, R.drawable.ic_walk, new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        AlohaHelper.logClick(AlohaHelper.ROUTING_PEDESTRIAN_SET);
        Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_PEDESTRIAN_SET);
        RoutingController.get().setRouterType(Framework.ROUTER_TYPE_PEDESTRIAN);
      }
    });

    View progressFrame = planFrame.findViewById(R.id.progress_frame);
    mProgressVehicle = (WheelProgressView) progressFrame.findViewById(R.id.progress_vehicle);
    mProgressPedestrian = (WheelProgressView) progressFrame.findViewById(R.id.progress_pedestrian);

    mPlanningLabel = planFrame.findViewById(R.id.planning);
    mErrorLabel = planFrame.findViewById(R.id.error);
    mDetailsFrame = planFrame.findViewById(R.id.details_frame);
    mNumbersFrame = planFrame.findViewById(R.id.numbers);
    mNumbersTime = (TextView) mNumbersFrame.findViewById(R.id.time);
    mNumbersDistance = (TextView) mNumbersFrame.findViewById(R.id.distance);
    mNumbersArrival = (TextView) mNumbersFrame.findViewById(R.id.arrival);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
    {
      View divider = planFrame.findViewById(R.id.details_divider);
      if (divider != null)
        UiUtils.invisible(divider);
    }

    setTitle(R.string.p2p_route_planning);

    mToggle.setImageDrawable(mToggleImage);
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
  public void onUpClick()
  {
    AlohaHelper.logClick(AlohaHelper.ROUTING_CANCEL);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_CANCEL);
    RoutingController.get().cancelPlanning();
  }

  private boolean checkFrameHeight()
  {
    if (mFrameHeight > 0)
      return true;

    mFrameHeight = mSlotFrame.getHeight();
    mToolbarHeight = mToolbar.getHeight();
    return (mFrameHeight > 0);
  }

  private void animateSlotFrame(int offset)
  {
    ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) mSlotFrame.getLayoutParams();
    lp.topMargin = (mToolbarHeight - offset);
    mSlotFrame.setLayoutParams(lp);
  }

  public void updatePoints()
  {
    mSlotFrame.update();
  }

  private void updateProgressLabels()
  {
    RoutingController.BuildState buildState = RoutingController.get().getBuildState();
    boolean idle = (RoutingController.get().isPlanning() &&
                    buildState == RoutingController.BuildState.NONE);
    if (mDetailsFrame != null)
      UiUtils.showIf(!idle, mDetailsFrame);

    boolean ready = (buildState == RoutingController.BuildState.BUILT);
    UiUtils.showIf(ready, mNumbersFrame);
    UiUtils.showIf(RoutingController.get().isBuilding(), mPlanningLabel);
    UiUtils.showIf(buildState == RoutingController.BuildState.ERROR, mErrorLabel);

    if (!ready)
      return;

    RoutingInfo rinfo = RoutingController.get().getCachedRoutingInfo();
    mNumbersTime.setText(RoutingController.formatRoutingTime(rinfo.totalTimeInSeconds, R.dimen.text_size_routing_number));
    mNumbersDistance.setText(rinfo.distToTarget + " " + rinfo.targetUnits);

    if (mNumbersArrival != null)
      mNumbersArrival.setText(MwmApplication.get().getString(R.string.routing_arrive,
                              RoutingController.formatArrivalTime(rinfo.totalTimeInSeconds)));
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
    AlohaHelper.logClick(AlohaHelper.ROUTING_TOGGLE);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_TOGGLE);
    showSlots(!mOpen, true);
  }

  protected void showSlots(final boolean show, final boolean animate)
  {
    if (!checkFrameHeight())
    {
      mFrame.post(new Runnable()
      {
        @Override
        public void run()
        {
          showSlots(show, animate);
        }
      });
      return;
    }

    mOpen = show;

    if (animate)
    {
      ValueAnimator animator = ValueAnimator.ofFloat(mOpen ? 1.0f : 0, mOpen ? 0 : 1.0f);
      animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
      {
        @Override
        public void onAnimationUpdate(ValueAnimator animation)
        {
          float fraction = (float)animation.getAnimatedValue();
          animateSlotFrame((int)(fraction * mFrameHeight));
          mToggleImage.setAngle((1.0f - fraction) * 180.0f);
        }
      });

      animator.setDuration(ANIM_TOGGLE);
      animator.start();
      mSlotFrame.fadeSlots(!mOpen);
    }
    else
    {
      animateSlotFrame(mOpen ? 0 : mFrameHeight);
      mToggleImage.setAngle(mOpen ? 180.0f : 0.0f);
      mSlotFrame.unfadeSlots();
    }
  }

  public void disableToggle()
  {
    UiUtils.hide(mToggle);
    showSlots(true, false);
  }

  public boolean isOpen()
  {
    return mOpen;
  }
}
