package com.mapswithme.maps.routing;

import android.app.Activity;
import android.content.res.Resources;
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
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
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
import com.mapswithme.maps.taxi.TaxiAdapter;
import com.mapswithme.maps.taxi.TaxiInfo;
import com.mapswithme.maps.taxi.TaxiLinks;
import com.mapswithme.maps.taxi.TaxiManager;
import com.mapswithme.maps.widget.DotPager;
import com.mapswithme.maps.widget.recycler.DotDividerItemDecoration;
import com.mapswithme.maps.widget.recycler.MultilineLayoutManager;
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
  private static final String STATE_ERROR = "error";

  @NonNull
  private final Activity mContext;
  @NonNull
  private final View mAltitudeChartFrame;
  @NonNull
  private final View mTransitFrame;
  @NonNull
  private final View mTaxiFrame;
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
  @NonNull
  private final DotDividerItemDecoration mTransitViewDecorator;
  @Nullable
  private TaxiInfo mTaxiInfo;
  @Nullable
  private TaxiInfo.Product mTaxiProduct;

  @Nullable
  private RoutingBottomMenuListener mListener;

  @NonNull
  static RoutingBottomMenuController newInstance(@NonNull Activity activity, @NonNull View frame,
                                                 @Nullable RoutingBottomMenuListener listener)
  {
    View altitudeChartFrame = getViewById(activity, frame, R.id.altitude_chart_panel);
    View transitFrame = getViewById(activity, frame, R.id.transit_panel);
    View taxiFrame = getViewById(activity, frame, R.id.taxi_panel);
    TextView error = (TextView) getViewById(activity, frame, R.id.error);
    Button start = (Button) getViewById(activity, frame, R.id.start);
    ImageView altitudeChart = (ImageView) getViewById(activity, frame, R.id.altitude_chart);
    TextView altitudeDifference = (TextView) getViewById(activity, frame, R.id.altitude_difference);
    View numbersFrame = getViewById(activity, frame, R.id.numbers);
    View actionFrame = getViewById(activity, frame, R.id.routing_action_frame);

    return new RoutingBottomMenuController(activity, altitudeChartFrame, transitFrame, taxiFrame,
                                           error, start, altitudeChart, altitudeDifference,
                                           numbersFrame, actionFrame, listener);
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
                                      @NonNull View transitFrame,
                                      @NonNull View taxiFrame,
                                      @NonNull TextView error,
                                      @NonNull Button start,
                                      @NonNull ImageView altitudeChart,
                                      @NonNull TextView altitudeDifference,
                                      @NonNull View numbersFrame,
                                      @NonNull View actionFrame,
                                      @Nullable RoutingBottomMenuListener listener)
  {
    mContext = context;
    mAltitudeChartFrame = altitudeChartFrame;
    mTransitFrame = transitFrame;
    mTaxiFrame = taxiFrame;
    mError = error;
    mStart = start;
    mAltitudeChart = altitudeChart;
    mAltitudeDifference = altitudeDifference;
    mNumbersFrame = numbersFrame;
    mActionFrame = actionFrame;
    mActionMessage = (TextView) actionFrame.findViewById(R.id.tv__message);
    mActionButton = actionFrame.findViewById(R.id.btn__my_position_use);
    mActionButton.setOnClickListener(this);
    View actionSearchButton = actionFrame.findViewById(R.id.btn__search_point);
    actionSearchButton.setOnClickListener(this);
    mActionIcon = (ImageView) mActionButton.findViewById(R.id.iv__icon);
    UiUtils.hide(mAltitudeChartFrame, mTaxiFrame, mActionFrame);
    mListener = listener;
    Drawable dividerDrawable = ContextCompat.getDrawable(mContext, R.drawable.dot_divider);
    Resources res = mContext.getResources();
    mTransitViewDecorator = new DotDividerItemDecoration(dividerDrawable, res.getDimensionPixelSize(R.dimen.margin_base),
                                                         res.getDimensionPixelSize(R.dimen.margin_half));
  }

  void showAltitudeChartAndRoutingDetails()
  {
    UiUtils.hide(mError, mTaxiFrame, mActionFrame, mTransitFrame);

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

  void showTaxiInfo(@NonNull TaxiInfo info)
  {
    UiUtils.hide(mError, mAltitudeChartFrame, mActionFrame, mTransitFrame);
    UiUtils.showTaxiIcon((ImageView) mTaxiFrame.findViewById(R.id.iv__logo), info.getType());
    final List<TaxiInfo.Product> products = info.getProducts();
    mTaxiInfo = info;
    mTaxiProduct = products.get(0);
    final PagerAdapter adapter = new TaxiAdapter(mContext, mTaxiInfo.getType(), products);
    DotPager pager = new DotPager.Builder(mContext, (ViewPager) mTaxiFrame.findViewById(R.id.pager),
                                          adapter)
        .setIndicatorContainer((ViewGroup) mTaxiFrame.findViewById(R.id.indicator))
        .setPageChangedListener(new DotPager.OnPageChangedListener()
        {
          @Override
          public void onPageChanged(int position)
          {
            mTaxiProduct = products.get(position);
          }
        }).build();
    pager.show();

    setStartButton();
    UiUtils.show(mTaxiFrame);
  }

  void showTransitInfo(@NonNull TransitRouteInfo info)
  {
    UiUtils.hide(mError, mAltitudeChartFrame, mActionFrame, mAltitudeChartFrame, mTaxiFrame);
    UiUtils.show(mTransitFrame);
    RecyclerView rv = (RecyclerView) mTransitFrame.findViewById(R.id.transit_recycler_view);
    TransitStepAdapter adapter = new TransitStepAdapter();
    rv.setLayoutManager(new MultilineLayoutManager());
    rv.setNestedScrollingEnabled(false);
    rv.removeItemDecoration(mTransitViewDecorator);
    rv.addItemDecoration(mTransitViewDecorator);
    rv.setAdapter(adapter);
    adapter.setItems(info.getTransitSteps());

    TextView totalTimeView = (TextView) mTransitFrame.findViewById(R.id.total_time);
    totalTimeView.setText(RoutingController.formatRoutingTime(mContext, (int) info.getTotalTime(),
                                                            R.dimen.text_size_routing_number));
    TextView distanceView = (TextView) mTransitFrame.findViewById(R.id.total_distance);
    distanceView.setText(info.getTotalPedestrianDistance() + " " + info.getTotalDistanceUnits());
  }

  void showAddStartFrame()
  {
    UiUtils.hide(mTaxiFrame, mError, mTransitFrame);
    UiUtils.show(mActionFrame);
    mActionMessage.setText(R.string.routing_add_start_point);
    mActionMessage.setTag(RoutePointInfo.ROUTE_MARK_START);
    if (LocationHelper.INSTANCE.getMyPosition() != null)
    {
      UiUtils.show(mActionButton);
      Drawable icon = ContextCompat.getDrawable(mContext, R.drawable.ic_my_location);
      int colorAccent = ContextCompat.getColor(mContext,
                                               UiUtils.getStyledResourceId(mContext, R.attr.colorAccent));
      mActionIcon.setImageDrawable(Graphics.tint(icon, colorAccent));
    }
    else
    {
      UiUtils.hide(mActionButton);
    }
  }

  void showAddFinishFrame()
  {
    UiUtils.hide(mTaxiFrame, mError, mTransitFrame);
    UiUtils.show(mActionFrame);
    mActionMessage.setText(R.string.routing_add_finish_point);
    mActionMessage.setTag(RoutePointInfo.ROUTE_MARK_FINISH);
    UiUtils.hide(mActionButton);
  }

  void hideActionFrame()
  {
    UiUtils.hide(mActionFrame);
  }

  void setStartButton()
  {
    if (RoutingController.get().isTaxiRouterType() && mTaxiInfo != null)
    {
      mStart.setText(Utils.isTaxiAppInstalled(mContext, mTaxiInfo.getType())
                     ? R.string.taxi_order : R.string.install_app);
      mStart.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          handleTaxiClick();
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

  private void handleTaxiClick()
  {
    if (mTaxiProduct == null || mTaxiInfo == null)
      return;

    boolean isTaxiInstalled = Utils.isTaxiAppInstalled(mContext, mTaxiInfo.getType());
    MapObject startPoint = RoutingController.get().getStartPoint();
    MapObject endPoint = RoutingController.get().getEndPoint();
    TaxiLinks links = TaxiManager.getTaxiLink(mTaxiProduct.getProductId(), mTaxiInfo.getType(),
                                              startPoint, endPoint);
    if (links != null)
    {
      Utils.launchTaxiApp(mContext, links, mTaxiInfo.getType());
      trackTaxiStatistics(mTaxiInfo.getType(), isTaxiInstalled);
    }
  }

  void showError(@StringRes int message)
  {
    showError(mError.getContext().getString(message));
  }

  private void showError(@NonNull String message)
  {
    UiUtils.hide(mTaxiFrame, mAltitudeChartFrame, mActionFrame, mTransitFrame);
    mError.setText(message);
    mError.setVisibility(View.VISIBLE);
    showStartButton(false);
  }

  void showStartButton(boolean show)
  {
    boolean result = show && (RoutingController.get().isBuilt()
                              || RoutingController.get().isTaxiRouterType());
    UiUtils.showIf(result, mStart);
  }

  void saveRoutingPanelState(@NonNull Bundle outState)
  {
    outState.putBoolean(STATE_ALTITUDE_CHART_SHOWN, UiUtils.isVisible(mAltitudeChartFrame));
    outState.putParcelable(STATE_TAXI_INFO, mTaxiInfo);
    if (UiUtils.isVisible(mError))
      outState.putString(STATE_ERROR, mError.getText().toString());
  }

  void restoreRoutingPanelState(@NonNull Bundle state)
  {
    if (state.getBoolean(STATE_ALTITUDE_CHART_SHOWN))
      showAltitudeChartAndRoutingDetails();

    TaxiInfo info = state.getParcelable(STATE_TAXI_INFO);
    if (info != null)
      showTaxiInfo(info);

    String error = state.getString(STATE_ERROR);
    if (!TextUtils.isEmpty(error))
      showError(error);
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

  private static void trackTaxiStatistics(@TaxiManager.TaxiType int type, boolean isTaxiAppInstalled)
  {
    MapObject from = RoutingController.get().getStartPoint();
    MapObject to = RoutingController.get().getEndPoint();
    Location location = LocationHelper.INSTANCE.getLastKnownLocation();
    Statistics.INSTANCE.trackTaxiInRoutePlanning(from, to, location, type, isTaxiAppInstalled);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
      case R.id.btn__my_position_use:
        if (mListener != null)
          mListener.onUseMyPositionAsStart();
        break;
      case R.id.btn__search_point:
        if (mListener != null)
        {
          @RoutePointInfo.RouteMarkType
          int pointType = (Integer) mActionMessage.getTag();
          mListener.onSearchRoutePoint(pointType);
        }
        break;
    }
  }
}
