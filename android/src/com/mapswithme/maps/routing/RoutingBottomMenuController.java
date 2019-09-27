package com.mapswithme.maps.routing;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.core.content.ContextCompat;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;
import androidx.recyclerview.widget.RecyclerView;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.AbsoluteSizeSpan;
import android.text.style.ForegroundColorSpan;
import android.text.style.StyleSpan;
import android.text.style.TypefaceSpan;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.taxi.TaxiAdapter;
import com.mapswithme.maps.taxi.TaxiInfo;
import com.mapswithme.maps.taxi.TaxiManager;
import com.mapswithme.maps.widget.DotPager;
import com.mapswithme.maps.widget.recycler.DotDividerItemDecoration;
import com.mapswithme.maps.widget.recycler.MultilineLayoutManager;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.SponsoredLinks;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

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
    int dividerRes = ThemeUtils.getResource(mContext, R.attr.transitStepDivider);
    Drawable dividerDrawable = ContextCompat.getDrawable(mContext, dividerRes);
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
    UiUtils.hide(mAltitudeChartFrame, mTransitFrame);
  }

  void showTaxiInfo(@NonNull TaxiInfo info)
  {
    UiUtils.hide(mError, mAltitudeChartFrame, mActionFrame, mTransitFrame);
    ImageView logo = mTaxiFrame.findViewById(R.id.iv__logo);
    logo.setImageResource(info.getType().getIcon());
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

  @SuppressLint("SetTextI18n")
  void showTransitInfo(@NonNull TransitRouteInfo info)
  {
    UiUtils.hide(mError, mAltitudeChartFrame, mActionFrame, mAltitudeChartFrame, mTaxiFrame);
    showStartButton(false);
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
    View dotView = mTransitFrame.findViewById(R.id.dot);
    View pedestrianIcon = mTransitFrame.findViewById(R.id.pedestrian_icon);
    TextView distanceView = (TextView) mTransitFrame.findViewById(R.id.total_distance);
    UiUtils.showIf(info.getTotalPedestrianTimeInSec() > 0, dotView, pedestrianIcon, distanceView);
    distanceView.setText(info.getTotalPedestrianDistance() + " " + info.getTotalPedestrianDistanceUnits());
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
      mStart.setText(Utils.isAppInstalled(mContext, mTaxiInfo.getType().getPackageName())
                     ? R.string.taxi_order : R.string.install_app);
      mStart.setOnClickListener(v -> handleTaxiClick());
    } else
    {
      mStart.setText(mContext.getText(R.string.p2p_start));
      mStart.setOnClickListener(v -> {
        if (mListener != null)
          mListener.onRoutingStart();
      });
    }

    showStartButton(true);
  }

  private void handleTaxiClick()
  {
    if (mTaxiProduct == null || mTaxiInfo == null)
      return;

    MapObject startPoint = RoutingController.get().getStartPoint();
    MapObject endPoint = RoutingController.get().getEndPoint();
    SponsoredLinks links = TaxiManager.getTaxiLink(mTaxiProduct.getProductId(), mTaxiInfo.getType(),
                                                   startPoint, endPoint);
    if (links != null)
      TaxiManager.launchTaxiApp(mContext, links, mTaxiInfo.getType());
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

    Spanned spanned = makeSpannedRoutingDetails(mContext, rinfo);
    TextView numbersTime = (TextView) mNumbersFrame.findViewById(R.id.time);
    numbersTime.setText(spanned);

    TextView numbersArrival = (TextView) mNumbersFrame.findViewById(R.id.arrival);
    if (numbersArrival != null)
    {
      String arrivalTime = RoutingController.formatArrivalTime(rinfo.totalTimeInSeconds);
      numbersArrival.setText(arrivalTime);
    }
  }

  @NonNull
  private static Spanned makeSpannedRoutingDetails(@NonNull Context context, @NonNull RoutingInfo routingInfo)

  {
    CharSequence time = RoutingController.formatRoutingTime(context,
                                                            routingInfo.totalTimeInSeconds,
                                                            R.dimen.text_size_routing_number);

    SpannableStringBuilder builder = new SpannableStringBuilder();
    initTimeBuilderSequence(context, time, builder);

    String dot = " • ";
    initDotBuilderSequence(context, dot, builder);

    String dist = routingInfo.distToTarget + " " + routingInfo.targetUnits;
    initDistanceBuilderSequence(context, dist, builder);

    return builder;
  }

  private static void initTimeBuilderSequence(@NonNull Context context, @NonNull CharSequence time,
                                              @NonNull SpannableStringBuilder builder)
  {
    builder.append(time);

    builder.setSpan(new TypefaceSpan(context.getResources().getString(R.string.robotoMedium)),
                    0,
                    builder.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new AbsoluteSizeSpan(context.getResources()
                                                .getDimensionPixelSize(R.dimen.text_size_routing_number)),
                    0,
                    builder.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new StyleSpan(Typeface.BOLD),
                    0,
                    builder.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new ForegroundColorSpan(ThemeUtils.getColor(context,
                                                                android.R.attr.textColorPrimary)),
                    0,
                    builder.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
  }

  private static void initDotBuilderSequence(@NonNull Context context, @NonNull String dot,
                                             @NonNull SpannableStringBuilder builder)
  {
    builder.append(dot);
    builder.setSpan(new TypefaceSpan(context.getResources().getString(R.string.robotoMedium)),
                    builder.length() - dot.length(),
                    builder.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new AbsoluteSizeSpan(context.getResources()
                                                .getDimensionPixelSize(R.dimen.text_size_routing_number)),
                    builder.length() - dot.length(),
                    builder.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new ForegroundColorSpan(ThemeUtils.getColor(context, R.attr.secondary)),
                    builder.length() - dot.length(),
                    builder.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
  }

  private static void initDistanceBuilderSequence(@NonNull Context context, @NonNull String arrivalTime,
                                                  @NonNull SpannableStringBuilder builder)
  {
    builder.append(arrivalTime);
    builder.setSpan(new TypefaceSpan(context.getResources().getString(R.string.robotoMedium)),
                    builder.length() - arrivalTime.length(),
                    builder.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new AbsoluteSizeSpan(context.getResources()
                                                .getDimensionPixelSize(R.dimen.text_size_routing_number)),
                    builder.length() - arrivalTime.length(),
                    builder.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new StyleSpan(Typeface.NORMAL),
                    builder.length() - arrivalTime.length(),
                    builder.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new ForegroundColorSpan(ThemeUtils.getColor(context, android.R.attr.textColorPrimary)),
                    builder.length() - arrivalTime.length(),
                    builder.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
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
