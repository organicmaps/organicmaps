package com.mapswithme.maps.routing;

import android.animation.ValueAnimator;
import android.app.Activity;
import android.graphics.Bitmap;
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
  private final WheelProgressView mProgressBicycle;
  private final View mAltitudeChartFrame;

  private final RotateDrawable mToggleImage = new RotateDrawable(R.drawable.ic_down);
  private int mFrameHeight;
  private int mToolbarHeight;
  private boolean mOpen;
  private boolean mAltitudeChartShown;

  private RadioButton setupRouterButton(@IdRes int buttonId, final @DrawableRes int iconRes, View.OnClickListener clickListener)
  {
    CompoundButton.OnCheckedChangeListener listener = new CompoundButton.OnCheckedChangeListener()
    {
      @Override
      public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
      {
        buttonView.setButtonDrawable(Graphics.tint(mActivity, iconRes, isChecked ? R.attr.routingButtonPressedHint
                                                                                 : R.attr.routingButtonHint));
      }
    };

    RadioButton rb = (RadioButton) mRouterTypes.findViewById(buttonId);
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
    mRouterTypes = (RadioGroup) mToolbar.findViewById(R.id.route_type);

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

    setupRouterButton(R.id.bicycle, R.drawable.ic_bicycle, new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        AlohaHelper.logClick(AlohaHelper.ROUTING_BICYCLE_SET);
        Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_BICYCLE_SET);
        RoutingController.get().setRouterType(Framework.ROUTER_TYPE_BICYCLE);
      }
    });

    View progressFrame = mToolbar.findViewById(R.id.progress_frame);
    mProgressVehicle = (WheelProgressView) progressFrame.findViewById(R.id.progress_vehicle);
    mProgressPedestrian = (WheelProgressView) progressFrame.findViewById(R.id.progress_pedestrian);
    mProgressBicycle = (WheelProgressView) progressFrame.findViewById(R.id.progress_bicycle);

    View altitudeChartFrame = mFrame.findViewById(R.id.altitude_chart_panel);
    if (altitudeChartFrame == null)
      altitudeChartFrame = mActivity.findViewById(R.id.altitude_chart_panel);

    mAltitudeChartFrame = altitudeChartFrame;
    UiUtils.hide(mAltitudeChartFrame);

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

    boolean ready = (buildState == RoutingController.BuildState.BUILT);

    if (!ready) 
    {
      hideAltitudeChartAndRoutingDetails();
      return;
    }

    showAltitudeChartAndRoutingDetails();
  }

  private void showAltitudeChartAndRoutingDetails()
  {
    final View numbersFrame = mAltitudeChartFrame.findViewById(R.id.numbers);
    TextView numbersTime = (TextView) numbersFrame.findViewById(R.id.time);
    TextView numbersDistance = (TextView) numbersFrame.findViewById(R.id.distance);
    TextView numbersArrival = (TextView) numbersFrame.findViewById(R.id.arrival);
    RoutingInfo rinfo = RoutingController.get().getCachedRoutingInfo();
    numbersTime.setText(RoutingController.formatRoutingTime(mFrame.getContext(), rinfo.totalTimeInSeconds,
                                                            R.dimen.text_size_routing_number));
    numbersDistance.setText(rinfo.distToTarget + " " + rinfo.targetUnits);

    if (numbersArrival != null)
    {
      String arrivalTime = RoutingController.formatArrivalTime(rinfo.totalTimeInSeconds);
      numbersArrival.setText(arrivalTime);
    }

    UiUtils.show(mAltitudeChartFrame);
    mAltitudeChartShown = true;
  }
  
  private void hideAltitudeChartAndRoutingDetails()
  {
    if (UiUtils.isHidden(mAltitudeChartFrame))
      return;

    UiUtils.hide(mAltitudeChartFrame);
    mAltitudeChartShown = false;
  }

  protected boolean isAltitudeChartShown()
  {
    return mAltitudeChartShown;
  }

  public void updateBuildProgress(int progress, @Framework.RouterType int router)
  {
    updateProgressLabels();
    UiUtils.invisible(mProgressVehicle, mProgressPedestrian, mProgressBicycle);
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
    else
    {
      mRouterTypes.check(R.id.bicycle);
      progressView = mProgressBicycle;
    }

    if (!RoutingController.get().isBuilding())
      return;

    UiUtils.show(progressView);
    progressView.setPending(progress == 0);
    if (progress != 0)
      progressView.setProgress(progress);
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

  protected boolean isVehicleRouteChecked()
  {
    return mRouterTypes.getCheckedRadioButtonId() == R.id.vehicle;
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

  public void showRouteAltitudeChart(boolean show)
  {
    ImageView altitudeChart = (ImageView) mFrame.findViewById(R.id.altitude_chart);
    showRouteAltitudeChartInternal(show, altitudeChart);
  }

  protected void showRouteAltitudeChartInternal(boolean show, ImageView altitudeChart)
  {
    if (!show)
    {
      UiUtils.hide(altitudeChart);
      return;
    }

    int chartWidth = UiUtils.dimen(mActivity, R.dimen.altitude_chart_image_width);
    int chartHeight = UiUtils.dimen(mActivity, R.dimen.altitude_chart_image_height);
    Bitmap bm = Framework.GenerateRouteAltitudeChart(chartWidth, chartHeight);
    if (bm != null)
    {
      altitudeChart.setImageBitmap(bm);
      UiUtils.show(altitudeChart);
    }
  }
}
