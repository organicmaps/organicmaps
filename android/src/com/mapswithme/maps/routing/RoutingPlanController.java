package com.mapswithme.maps.routing;

import android.animation.ValueAnimator;
import android.app.Activity;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.support.annotation.DrawableRes;
import android.support.annotation.IdRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.uber.UberAdapter;
import com.mapswithme.maps.uber.UberInfo;
import com.mapswithme.maps.uber.UberLinks;
import com.mapswithme.maps.widget.DotPager;
import com.mapswithme.maps.widget.RotateDrawable;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

public class RoutingPlanController extends ToolbarController
{
  static final int ANIM_TOGGLE = MwmApplication.get().getResources().getInteger(R.integer.anim_slots_toggle);
  private static final String STATE_ALTITUDE_CHART_SHOWN = "altitude_chart_shown";
  private static final String STATE_TAXI_INFO = "taxi_info";


  protected final View mFrame;
  private final ImageView mToggle;
  private final SlotFrame mSlotFrame;
  private final RadioGroup mRouterTypes;
  private final WheelProgressView mProgressVehicle;
  private final WheelProgressView mProgressPedestrian;
  private final WheelProgressView mProgressBicycle;
  private final WheelProgressView mProgressTaxi;
  private final View mAltitudeChartFrame;
  private final View mUberFrame;

  private final RotateDrawable mToggleImage = new RotateDrawable(R.drawable.ic_down);
  private int mFrameHeight;
  private int mToolbarHeight;
  private boolean mOpen;
  @Nullable
  private UberInfo mUberInfo;

  @Nullable
  private UberInfo.Product mUberProduct;

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

  RoutingPlanController(View root, Activity activity)
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

    setupRouterButton(R.id.taxi, R.drawable.ic_taxi, new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        AlohaHelper.logClick(AlohaHelper.ROUTING_TAXI_SET);
        Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_TAXI_SET);
        RoutingController.get().setRouterType(Framework.ROUTER_TYPE_TAXI);
      }
    });

    View progressFrame = mToolbar.findViewById(R.id.progress_frame);
    mProgressVehicle = (WheelProgressView) progressFrame.findViewById(R.id.progress_vehicle);
    mProgressPedestrian = (WheelProgressView) progressFrame.findViewById(R.id.progress_pedestrian);
    mProgressBicycle = (WheelProgressView) progressFrame.findViewById(R.id.progress_bicycle);
    mProgressTaxi = (WheelProgressView) progressFrame.findViewById(R.id.progress_taxi);

    mAltitudeChartFrame = getViewById(R.id.altitude_chart_panel);
    UiUtils.hide(mAltitudeChartFrame);

    mUberFrame = getViewById(R.id.uber_panel);
    UiUtils.hide(mUberFrame);

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

    if (!isTaxiRouteChecked())
      setStartButton();
    showAltitudeChartAndRoutingDetails();
  }

  private void showAltitudeChartAndRoutingDetails()
  {
    if (isTaxiRouteChecked())
      return;

    UiUtils.hide(getViewById(R.id.error));
    UiUtils.hide(mUberFrame);

    showRouteAltitudeChart();
    showRoutingDetails();
    UiUtils.show(mAltitudeChartFrame);
  }

  private void showRoutingDetails()
  {
    final View numbersFrame = mAltitudeChartFrame.findViewById(R.id.numbers);
    final RoutingInfo rinfo = RoutingController.get().getCachedRoutingInfo();
    if (rinfo == null)
    {
      UiUtils.hide(numbersFrame);
      return;
    }

    TextView numbersTime = (TextView) numbersFrame.findViewById(R.id.time);
    TextView numbersDistance = (TextView) numbersFrame.findViewById(R.id.distance);
    TextView numbersArrival = (TextView) numbersFrame.findViewById(R.id.arrival);
    numbersTime.setText(RoutingController.formatRoutingTime(mFrame.getContext(), rinfo.totalTimeInSeconds,
                                                            R.dimen.text_size_routing_number));
    numbersDistance.setText(rinfo.distToTarget + " " + rinfo.targetUnits);

    if (numbersArrival != null)
    {
      String arrivalTime = RoutingController.formatArrivalTime(rinfo.totalTimeInSeconds);
      numbersArrival.setText(arrivalTime);
    }
  }

  private void hideAltitudeChartAndRoutingDetails()
  {
    if (UiUtils.isHidden(mAltitudeChartFrame))
      return;

    UiUtils.hide(mAltitudeChartFrame);
  }

  public void updateBuildProgress(int progress, @Framework.RouterType int router)
  {
    UiUtils.invisible(mProgressVehicle, mProgressPedestrian, mProgressBicycle, mProgressTaxi);
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
    else
    {
      mRouterTypes.check(R.id.bicycle);
      progressView = mProgressBicycle;
    }

    updateProgressLabels();

    if (!RoutingController.get().isBuilding() || RoutingController.get().isUberInfoObtained())
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

  void showSlots(final boolean show, final boolean animate)
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

  private boolean isVehicleRouteChecked()
  {
    return mRouterTypes.getCheckedRadioButtonId() == R.id.vehicle;
  }

  private boolean isTaxiRouteChecked()
  {
    return mRouterTypes.getCheckedRadioButtonId() == R.id.taxi;
  }

  void disableToggle()
  {
    UiUtils.hide(mToggle);
    showSlots(true, false);
  }

  public boolean isOpen()
  {
    return mOpen;
  }

  public void showRouteAltitudeChart()
  {
    ImageView altitudeChart = (ImageView) mFrame.findViewById(R.id.altitude_chart);
    showRouteAltitudeChartInternal(altitudeChart);
  }

  void showRouteAltitudeChartInternal(@NonNull ImageView altitudeChart)
  {
    if (isVehicleRouteChecked())
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

  public void showUberInfo(@NonNull UberInfo info)
  {
    final UberInfo.Product[] products = info.getProducts();
    if (products == null || products.length == 0)
    {
      UiUtils.hide(mUberFrame);
      showError(R.string.taxi_not_found);
      return;
    }

    UiUtils.hide(getViewById(R.id.error));
    mUberInfo = info;
    mUberProduct = products[0];
    final PagerAdapter adapter = new UberAdapter(mActivity, products);
    DotPager pager = new DotPager.Builder(mActivity, (ViewPager) mUberFrame.findViewById(R.id.pager), adapter)
        .setIndicatorContainer((ViewGroup) mUberFrame.findViewById(R.id.indicator))
        .setPageChangedListener(new DotPager.OnPageChangedListener()
        {
          @Override
          public void onPageChanged(int position)
          {
            mUberProduct = products[position];
          }
        }).build();
    pager.show();

    setStartButton();

    UiUtils.hide(mAltitudeChartFrame);
    UiUtils.show(mUberFrame);
  }

  private void showError(@StringRes int message)
  {
    TextView error = (TextView) getViewById(R.id.error);
    error.setText(message);
    error.setVisibility(View.VISIBLE);
    getViewById(R.id.start).setVisibility(View.GONE);
  }

  @NonNull
  private View getViewById(@IdRes int resourceId)
  {
    View view = mFrame.findViewById(resourceId);
    if (view == null)
      view = mActivity.findViewById(resourceId);
    return view;
  }

  void saveRoutingPanelState(@NonNull Bundle outState)
  {
    outState.putBoolean(STATE_ALTITUDE_CHART_SHOWN, UiUtils.isVisible(mAltitudeChartFrame));
    outState.putParcelable(STATE_TAXI_INFO, mUberInfo);
  }

  void restoreRoutingPanelState(@NonNull Bundle state)
  {
    if (state.getBoolean(STATE_ALTITUDE_CHART_SHOWN))
      showRouteAltitudeChart();

    UberInfo info = state.getParcelable(STATE_TAXI_INFO);
    if (info != null)
      showUberInfo(info);
  }

  private void setStartButton()
  {
    Button start = (Button) getViewById(R.id.start);

    if (isTaxiRouteChecked())
    {
      start.setText(R.string.taxi_order);
      start.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          if (mUberProduct != null)
          {
            UberLinks links = RoutingController.get().getUberLink(mUberProduct.getProductId());
            Utils.launchUber(mActivity, links);
          }
        }
      });
    } else
    {
      start.setText(mActivity.getText(R.string.p2p_start));
      start.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          ((MwmActivity)mActivity).closeMenu(Statistics.EventName.ROUTING_START, AlohaHelper.ROUTING_START, new Runnable()
          {
            @Override
            public void run()
            {
              RoutingController.get().start();
            }
          });
        }
      });
    }

    UiUtils.updateAccentButton(start);
    UiUtils.show(start);
  }

}
