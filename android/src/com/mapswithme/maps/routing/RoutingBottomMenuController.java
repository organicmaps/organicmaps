package com.mapswithme.maps.routing;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.os.Bundle;
import android.support.annotation.IdRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.content.ContextCompat;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.uber.UberAdapter;
import com.mapswithme.maps.uber.UberInfo;
import com.mapswithme.maps.uber.UberLinks;
import com.mapswithme.maps.widget.DotPager;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

import java.util.List;
import java.util.Locale;

final class RoutingBottomMenuController implements View.OnClickListener
{
  private static final String STATE_ALTITUDE_CHART_SHOWN = "altitude_chart_shown";
  private static final String STATE_TAXI_INFO = "taxi_info";

  @NonNull
  private final Activity mContext;
  @NonNull
  private final View mAltitudeChartFrame;
  @NonNull
  private final View mUberFrame;
  @NonNull
  private final TextView mError;
  @NonNull
  private final Button mStart;
  @NonNull
  private final ImageView mAltitudeChart;
  @NonNull
  private final TextView mAltitudeDifference;
  @NonNull
  private final View mNumbersFrame;
  @NonNull
  private final View mActionFrame;
  @NonNull
  private final TextView mActionMessage;
  @NonNull
  private final View mActionButton;
  @NonNull
  private final ImageView mActionIcon;

  @Nullable
  private UberInfo mUberInfo;
  @Nullable
  private UberInfo.Product mUberProduct;

  @NonNull
  static RoutingBottomMenuController newInstance(@NonNull Activity activity, @NonNull View frame)
  {
    View altitudeChartFrame = getViewById(activity, frame, R.id.altitude_chart_panel);
    View uberFrame = getViewById(activity, frame, R.id.uber_panel);
    TextView error = (TextView) getViewById(activity, frame, R.id.error);
    Button start = (Button) getViewById(activity, frame, R.id.start);
    ImageView altitudeChart = (ImageView) getViewById(activity, frame, R.id.altitude_chart);
    TextView altitudeDifference = (TextView) getViewById(activity, frame, R.id.altitude_difference);
    View numbersFrame = getViewById(activity, frame, R.id.numbers);
    View actionFrame = getViewById(activity, frame, R.id.routing_action_frame);

    return new RoutingBottomMenuController(activity, altitudeChartFrame,
                                           uberFrame, error, start,
                                           altitudeChart,
                                           altitudeDifference,
                                           numbersFrame, actionFrame);
  }

  @NonNull
  private static View getViewById(@NonNull Activity activity, @NonNull View frame,
                                  @IdRes int resourceId)
  {
    View view = frame.findViewById(resourceId);
    return view == null ? activity.findViewById(resourceId) : view;
  }

  private RoutingBottomMenuController(@NonNull Activity context,
                                      @NonNull View altitudeChartFrame,
                                      @NonNull View uberFrame,
                                      @NonNull TextView error,
                                      @NonNull Button start,
                                      @NonNull ImageView altitudeChart,
                                      @NonNull TextView altitudeDifference,
                                      @NonNull View numbersFrame,
                                      @NonNull View actionFrame)
  {
    mContext = context;
    mAltitudeChartFrame = altitudeChartFrame;
    mUberFrame = uberFrame;
    mError = error;
    mStart = start;
    mAltitudeChart = altitudeChart;
    mAltitudeDifference = altitudeDifference;
    mNumbersFrame = numbersFrame;
    mActionFrame = actionFrame;
    mActionMessage = (TextView) actionFrame.findViewById(R.id.tv__message);
    mActionButton = actionFrame.findViewById(R.id.btn__my_position_use);
    mActionIcon = (ImageView) mActionButton.findViewById(R.id.iv__icon);
    mActionButton.setOnClickListener(this);
    mActionFrame.setOnTouchListener(new View.OnTouchListener()
    {
      @Override
      public boolean onTouch(View v, MotionEvent event)
      {
        return !(UiUtils.isVisible(mActionButton) && UiUtils.isViewTouched(event, mActionButton));
      }
    });
    UiUtils.hide(mAltitudeChartFrame, mUberFrame, mActionFrame);
  }

  void showAltitudeChartAndRoutingDetails()
  {
    UiUtils.hide(mError, mUberFrame, mActionFrame);

    showRouteAltitudeChart();
    showRoutingDetails();
    UiUtils.show(mAltitudeChartFrame);
  }

  void hideAltitudeChartAndRoutingDetails()
  {
    if (UiUtils.isHidden(mAltitudeChartFrame))
      return;

    UiUtils.hide(mAltitudeChartFrame);
  }

  void showUberInfo(@NonNull UberInfo info)
  {
    UiUtils.hide(mError, mAltitudeChartFrame, mActionFrame);

    final List<UberInfo.Product> products = info.getProducts();
    mUberInfo = info;
    mUberProduct = products.get(0);
    final PagerAdapter adapter = new UberAdapter(mContext, products);
    DotPager pager = new DotPager.Builder(mContext, (ViewPager) mUberFrame.findViewById(R.id.pager),
                                          adapter)
        .setIndicatorContainer((ViewGroup) mUberFrame.findViewById(R.id.indicator))
        .setPageChangedListener(new DotPager.OnPageChangedListener()
        {
          @Override
          public void onPageChanged(int position)
          {
            mUberProduct = products.get(position);
          }
        }).build();
    pager.show();

    setStartButton();
    UiUtils.show(mUberFrame);
  }

  void showAddStartFrame()
  {
    UiUtils.show(mActionFrame, mActionButton);
    mActionMessage.setText(R.string.routing_add_start_point);
    Drawable icon = ContextCompat.getDrawable(mContext, R.drawable.ic_my_location);
    int colorAccent = ContextCompat.getColor(mContext,
                                             UiUtils.getStyledResourceId(mContext, R.attr.colorAccent));
    mActionIcon.setImageDrawable(Graphics.tint(icon, colorAccent));
  }

  void showAddFinishFrame()
  {
    UiUtils.show(mActionFrame);
    mActionMessage.setText(R.string.routing_add_finish_point);
    UiUtils.hide(mActionButton);
  }

  void hideActionFrame()
  {
    UiUtils.hide(mActionFrame);
  }

  void setStartButton()
  {
    if (RoutingController.get().isTaxiRouterType())
    {
      final boolean isUberInstalled = Utils.isUberInstalled(mContext);
      mStart.setText(isUberInstalled ? R.string.taxi_order : R.string.install_app);
      mStart.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          if (mUberProduct != null)
          {
            UberLinks links = RoutingController.get().getUberLink(mUberProduct.getProductId());
            if (links != null)
            {
              Utils.launchUber(mContext, links);
              trackUberStatistics(isUberInstalled);
            }
          }
        }
      });
    } else
    {
      mStart.setText(mContext.getText(R.string.p2p_start));
      mStart.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          ((MwmActivity)mContext).closeMenu(Statistics.EventName.ROUTING_START, AlohaHelper.ROUTING_START, new Runnable()
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

    UiUtils.updateAccentButton(mStart);
    showStartButton(true);
  }

  void showError(@StringRes int message)
  {
    UiUtils.hide(mUberFrame, mAltitudeChartFrame);
    mError.setText(message);
    mError.setVisibility(View.VISIBLE);
    showStartButton(false);
  }

  void showStartButton(boolean show)
  {
    UiUtils.showIf(show, mStart);
  }

  void saveRoutingPanelState(@NonNull Bundle outState)
  {
    outState.putBoolean(STATE_ALTITUDE_CHART_SHOWN, UiUtils.isVisible(mAltitudeChartFrame));
    outState.putParcelable(STATE_TAXI_INFO, mUberInfo);
  }

  void restoreRoutingPanelState(@NonNull Bundle state)
  {
    if (state.getBoolean(STATE_ALTITUDE_CHART_SHOWN))
      showAltitudeChartAndRoutingDetails();

    UberInfo info = state.getParcelable(STATE_TAXI_INFO);
    if (info != null)
      showUberInfo(info);
  }

  private void showRouteAltitudeChart()
  {
    if (RoutingController.get().isVehicleRouterType())
    {
      UiUtils.hide(mAltitudeChart, mAltitudeDifference);
      return;
    }

    int chartWidth = UiUtils.dimen(mContext, R.dimen.altitude_chart_image_width);
    int chartHeight = UiUtils.dimen(mContext, R.dimen.altitude_chart_image_height);
    Framework.RouteAltitudeLimits limits = new Framework.RouteAltitudeLimits();
    Bitmap bm = Framework.generateRouteAltitudeChart(chartWidth, chartHeight, limits);
    if (bm != null)
    {
      mAltitudeChart.setImageBitmap(bm);
      UiUtils.show(mAltitudeChart);
      String meter = mAltitudeDifference.getResources().getString(R.string.meter);
      String foot = mAltitudeDifference.getResources().getString(R.string.foot);
      mAltitudeDifference.setText(String.format(Locale.getDefault(), "%d %s",
                                                limits.maxRouteAltitude - limits.minRouteAltitude,
                                                limits.isMetricUnits ? meter : foot));
      Drawable icon = ContextCompat.getDrawable(mContext,
                                                R.drawable.ic_altitude_difference);
      int colorAccent = ContextCompat.getColor(mContext,
          UiUtils.getStyledResourceId(mContext, R.attr.colorAccent));
      mAltitudeDifference.setCompoundDrawablesWithIntrinsicBounds(Graphics.tint(icon, colorAccent),
                                                                  null, null, null);
      UiUtils.show(mAltitudeDifference);
    }
  }

  private void showRoutingDetails()
  {
    final RoutingInfo rinfo = RoutingController.get().getCachedRoutingInfo();
    if (rinfo == null)
    {
      UiUtils.hide(mNumbersFrame);
      return;
    }

    TextView numbersTime = (TextView) mNumbersFrame.findViewById(R.id.time);
    TextView numbersDistance = (TextView) mNumbersFrame.findViewById(R.id.distance);
    TextView numbersArrival = (TextView) mNumbersFrame.findViewById(R.id.arrival);
    numbersTime.setText(RoutingController.formatRoutingTime(mContext, rinfo.totalTimeInSeconds,
                                                            R.dimen.text_size_routing_number));
    numbersDistance.setText(rinfo.distToTarget + " " + rinfo.targetUnits);

    if (numbersArrival != null)
    {
      String arrivalTime = RoutingController.formatArrivalTime(rinfo.totalTimeInSeconds);
      numbersArrival.setText(arrivalTime);
    }
  }

  private static void trackUberStatistics(boolean isUberInstalled)
  {
    MapObject from = RoutingController.get().getStartPoint();
    MapObject to = RoutingController.get().getEndPoint();
    Location location = LocationHelper.INSTANCE.getLastKnownLocation();
    Statistics.INSTANCE.trackUber(from, to, location, isUberInstalled);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
      case R.id.btn__my_position_use:
        RoutingController.get().setStartPoint(LocationHelper.INSTANCE.getMyPosition());
        break;
    }
  }
}
