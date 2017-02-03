package com.mapswithme.maps.routing;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.os.Bundle;
import android.support.annotation.DrawableRes;
import android.support.annotation.IdRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.content.ContextCompat;
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
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.uber.Uber;
import com.mapswithme.maps.uber.UberAdapter;
import com.mapswithme.maps.uber.UberInfo;
import com.mapswithme.maps.uber.UberLinks;
import com.mapswithme.maps.widget.DotPager;
import com.mapswithme.maps.widget.RotateDrawable;
import com.mapswithme.maps.widget.RoutingToolbarButton;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

import java.util.Locale;

public class RoutingPlanController extends ToolbarController implements SlotFrame.SlotClickListener
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
  int mFrameHeight;
  private int mToolbarHeight;
  private boolean mOpen;
  @Nullable
  private UberInfo mUberInfo;

  @Nullable
  private UberInfo.Product mUberProduct;

  @Nullable
  private OnToggleListener mToggleListener;

  @Nullable
  private  SearchPoiTransitionListener mPoiTransitionListener;

  public interface OnToggleListener
  {
    void onToggle(boolean state);
  }

  public interface SearchPoiTransitionListener
  {
    void animateSearchPoiTransition(@NonNull final Rect startRect,
                                    @Nullable final Runnable runnable);
  }

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

  RoutingPlanController(View root, Activity activity)
  {
    super(root, activity);
    mFrame = root;

    mToggle = (ImageView) mToolbar.findViewById(R.id.toggle);
    mSlotFrame = (SlotFrame) root.findViewById(R.id.slots);
    mSlotFrame.setSlotClickListener(this);
    mRouterTypes = (RadioGroup) mToolbar.findViewById(R.id.route_type);

    setupRouterButton(R.id.vehicle, R.drawable.ic_car, new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        AlohaHelper.logClick(AlohaHelper.ROUTING_VEHICLE_SET);
        Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_VEHICLE_SET);
        RoutingController.get().setRouterType(Framework.ROUTER_TYPE_VEHICLE);
      }
    });

    setupRouterButton(R.id.pedestrian, R.drawable.ic_pedestrian, new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        AlohaHelper.logClick(AlohaHelper.ROUTING_PEDESTRIAN_SET);
        Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_PEDESTRIAN_SET);
        RoutingController.get().setRouterType(Framework.ROUTER_TYPE_PEDESTRIAN);
      }
    });

    setupRouterButton(R.id.bicycle, R.drawable.ic_bike, new View.OnClickListener()
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
    RoutingController.get().cancel();
  }

  @Override
  public void onSlotClicked(final int order, @NonNull Rect rect)
  {
    if (mPoiTransitionListener != null)
    {
      mPoiTransitionListener.animateSearchPoiTransition(rect, new Runnable()
      {
        @Override
        public void run()
        {
          RoutingController.get().searchPoi(order);
        }
      });
    }
    else
    {
      RoutingController.get().searchPoi(order);
    }
  }

  public void setPoiTransitionListener(@Nullable SearchPoiTransitionListener poiTransitionListener)
  {
    mPoiTransitionListener = poiTransitionListener;
  }

  boolean checkFrameHeight()
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

    if (!isTaxiRouterType())
      setStartButton();
    showAltitudeChartAndRoutingDetails();
  }

  private void showAltitudeChartAndRoutingDetails()
  {
    if (isTaxiRouterType())
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

    RoutingToolbarButton button = (RoutingToolbarButton)mRouterTypes
        .findViewById(mRouterTypes.getCheckedRadioButtonId());
    button.progress();

    updateProgressLabels();

    if (RoutingController.get().isUberRequestHandled())
    {
      if (!RoutingController.get().isInternetConnected())
      {
        showNoInternetError();
        return;
      }
      button.complete();
      return;
    }

    if (!RoutingController.get().isBuilding() && !RoutingController.get().isUberPlanning())
    {
      button.complete();
      return;
    }

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
      animator.addListener(new UiUtils.SimpleAnimatorListener() {
        @Override
        public void onAnimationEnd(Animator animation)
        {
          if (mToggleListener != null)
            mToggleListener.onToggle(mOpen);
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
      mSlotFrame.post(new Runnable()
      {
        @Override
        public void run()
        {
          if (mToggleListener != null)
            mToggleListener.onToggle(mOpen);
        }
      });
    }
  }

  private boolean isVehicleRouterType()
  {
    return RoutingController.get().isVehicleRouterType();
  }

  private boolean isTaxiRouterType()
  {
    return RoutingController.get().isTaxiRouterType();
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
    TextView altitudeDifference = (TextView) mAltitudeChartFrame.findViewById(R.id.altitude_difference);
    showRouteAltitudeChartInternal(altitudeChart, altitudeDifference);
  }

  void showRouteAltitudeChartInternal(@NonNull ImageView altitudeChart,
                                      @NonNull TextView altitudeDifference)
  {
    if (isVehicleRouterType())
    {
      UiUtils.hide(altitudeChart);
      UiUtils.hide(altitudeDifference);
      return;
    }

    int chartWidth = UiUtils.dimen(mActivity, R.dimen.altitude_chart_image_width);
    int chartHeight = UiUtils.dimen(mActivity, R.dimen.altitude_chart_image_height);
    Framework.RouteAltitudeLimits limits = new Framework.RouteAltitudeLimits();
    Bitmap bm = Framework.generateRouteAltitudeChart(chartWidth, chartHeight, limits);
    if (bm != null)
    {
      altitudeChart.setImageBitmap(bm);
      UiUtils.show(altitudeChart);
      String meter = altitudeDifference.getResources().getString(R.string.meter);
      String foot = altitudeDifference.getResources().getString(R.string.foot);
      altitudeDifference.setText(String.format(Locale.getDefault(), "%d %s",
                                               limits.maxRouteAltitude - limits.minRouteAltitude,
                                               limits.isMetricUnits ? meter : foot));
      Drawable icon = ContextCompat.getDrawable(altitudeDifference.getContext(),
                                                R.drawable.ic_altitude_difference);
      int colorAccent = ContextCompat.getColor(altitudeDifference.getContext(),
                             UiUtils.getStyledResourceId(altitudeDifference.getContext(), R.attr.colorAccent));
      altitudeDifference.setCompoundDrawablesWithIntrinsicBounds(Graphics.tint(icon,colorAccent),
                                                                 null, null, null);
      UiUtils.show(altitudeDifference);
    }
  }

  public void showUberInfo(@NonNull UberInfo info)
  {
    UiUtils.hide(getViewById(R.id.error), mAltitudeChartFrame);

    final UberInfo.Product[] products = info.getProducts();
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
    UiUtils.show(mUberFrame);
  }

  public void showUberError(@NonNull Uber.ErrorCode code)
  {
    switch (code)
    {
      case NoProducts:
        showError(R.string.taxi_not_found);
        break;
      case RemoteError:
        showError(R.string.dialog_taxi_error);
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
    UiUtils.hide(mUberFrame, mAltitudeChartFrame);
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

    if (isTaxiRouterType())
    {
      final boolean isUberInstalled = Utils.isUberInstalled(mActivity);
      start.setText(isUberInstalled ? R.string.taxi_order : R.string.install_app);
      start.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          if (mUberProduct != null)
          {
            UberLinks links = RoutingController.get().getUberLink(mUberProduct.getProductId());
            Utils.launchUber(mActivity, links);
            trackUberStatistics(isUberInstalled);
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

  private static void trackUberStatistics(boolean isUberInstalled)
  {
    MapObject from = RoutingController.get().getStartPoint();
    MapObject to = RoutingController.get().getEndPoint();
    Location location = LocationHelper.INSTANCE.getLastKnownLocation();
    Statistics.INSTANCE.trackUber(from, to, location, isUberInstalled);
  }

  public int getHeight()
  {
    return mFrame.getHeight();
  }

  public void setOnToggleListener(@Nullable OnToggleListener listener)
  {
    mToggleListener = listener;
  }
}
