package app.organicmaps.routing;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.AbsoluteSizeSpan;
import android.text.style.ForegroundColorSpan;
import android.text.style.StyleSpan;
import android.text.style.TypefaceSpan;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;

import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.widget.recycler.DotDividerItemDecoration;
import app.organicmaps.widget.recycler.MultilineLayoutManager;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;

import java.util.Locale;

final class RoutingBottomMenuController implements View.OnClickListener
{
  private static final String STATE_ALTITUDE_CHART_SHOWN = "altitude_chart_shown";
  private static final String STATE_ERROR = "error";

  @NonNull
  private final Activity mContext;
  @NonNull
  private final View mAltitudeChartFrame;
  @NonNull
  private final View mTransitFrame;
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
  private final RoutingBottomMenuListener mListener;

  @NonNull
  static RoutingBottomMenuController newInstance(@NonNull Activity activity, @NonNull View frame,
                                                 @Nullable RoutingBottomMenuListener listener)
  {
    View altitudeChartFrame = getViewById(activity, frame, R.id.altitude_chart_panel);
    View transitFrame = getViewById(activity, frame, R.id.transit_panel);
    TextView error = (TextView) getViewById(activity, frame, R.id.error);
    Button start = (Button) getViewById(activity, frame, R.id.start);
    ImageView altitudeChart = (ImageView) getViewById(activity, frame, R.id.altitude_chart);
    TextView altitudeDifference = (TextView) getViewById(activity, frame, R.id.altitude_difference);
    View numbersFrame = getViewById(activity, frame, R.id.numbers);
    View actionFrame = getViewById(activity, frame, R.id.routing_action_frame);

    return new RoutingBottomMenuController(activity, altitudeChartFrame, transitFrame, error, start, altitudeChart, altitudeDifference,
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
    mError = error;
    mStart = start;
    mAltitudeChart = altitudeChart;
    mAltitudeDifference = altitudeDifference;
    mNumbersFrame = numbersFrame;
    mActionFrame = actionFrame;
    mActionMessage = actionFrame.findViewById(R.id.tv__message);
    mActionButton = actionFrame.findViewById(R.id.btn__my_position_use);
    mActionButton.setOnClickListener(this);
    View actionSearchButton = actionFrame.findViewById(R.id.btn__search_point);
    actionSearchButton.setOnClickListener(this);
    mActionIcon = mActionButton.findViewById(R.id.iv__icon);
    UiUtils.hide(mAltitudeChartFrame, mActionFrame);
    mListener = listener;
    int dividerRes = ThemeUtils.getResource(mContext, R.attr.transitStepDivider);
    Drawable dividerDrawable = ContextCompat.getDrawable(mContext, dividerRes);
    Resources res = mContext.getResources();
    mTransitViewDecorator = new DotDividerItemDecoration(dividerDrawable, res.getDimensionPixelSize(R.dimen.margin_base),
                                                         res.getDimensionPixelSize(R.dimen.margin_half));
  }

  void showAltitudeChartAndRoutingDetails()
  {
    UiUtils.hide(mError, mActionFrame, mAltitudeChart, mAltitudeDifference, mTransitFrame);

    if (!RoutingController.get().isVehicleRouterType() &&
        !RoutingController.get().isHelicopterRouterType())
      showRouteAltitudeChart();
    showRoutingDetails();
    UiUtils.show(mAltitudeChartFrame);
  }

  void hideAltitudeChartAndRoutingDetails()
  {
    UiUtils.hide(mAltitudeChartFrame, mTransitFrame);
  }

  @SuppressLint("SetTextI18n")
  void showTransitInfo(@NonNull TransitRouteInfo info)
  {
    UiUtils.hide(mError, mAltitudeChartFrame, mActionFrame, mAltitudeChartFrame);
    showStartButton(false);
    UiUtils.show(mTransitFrame);
    RecyclerView rv = mTransitFrame.findViewById(R.id.transit_recycler_view);
    TransitStepAdapter adapter = new TransitStepAdapter();
    rv.setLayoutManager(new MultilineLayoutManager());
    rv.setNestedScrollingEnabled(false);
    rv.removeItemDecoration(mTransitViewDecorator);
    rv.addItemDecoration(mTransitViewDecorator);
    rv.setAdapter(adapter);
    adapter.setItems(info.getTransitSteps());

    TextView totalTimeView = mTransitFrame.findViewById(R.id.total_time);
    totalTimeView.setText(RoutingController.formatRoutingTime(mContext, info.getTotalTime(),
                                                            R.dimen.text_size_routing_number));
    View dotView = mTransitFrame.findViewById(R.id.dot);
    View pedestrianIcon = mTransitFrame.findViewById(R.id.pedestrian_icon);
    TextView distanceView = mTransitFrame.findViewById(R.id.total_distance);
    UiUtils.showIf(info.getTotalPedestrianTimeInSec() > 0, dotView, pedestrianIcon, distanceView);
    distanceView.setText(info.getTotalPedestrianDistance() + " " + info.getTotalPedestrianDistanceUnits());
  }

  void showAddStartFrame()
  {
    UiUtils.hide(mError, mTransitFrame);
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
    UiUtils.hide(mError, mTransitFrame);
    UiUtils.show(mActionFrame);
    mActionMessage.setText(R.string.routing_add_finish_point);
    mActionMessage.setTag(RoutePointInfo.ROUTE_MARK_FINISH);
    UiUtils.hide(mActionButton);
  }

  void hideActionFrame()
  {
    UiUtils.hide(mActionFrame);
  }

  void setStartButton(boolean show)
  {
    if (show) {
      mStart.setText(mContext.getText(R.string.p2p_start));
      mStart.setOnClickListener(v -> {
        if (mListener != null)
          mListener.onRoutingStart();
      });
    }

    showStartButton(show);
  }

  private void showError(@NonNull String message)
  {
    UiUtils.hide(mAltitudeChartFrame, mActionFrame, mTransitFrame);
    mError.setText(message);
    mError.setVisibility(View.VISIBLE);
    showStartButton(false);
  }

  void showStartButton(boolean show)
  {
    boolean result = show && RoutingController.get().isBuilt();
    UiUtils.showIf(result, mStart);
  }

  void saveRoutingPanelState(@NonNull Bundle outState)
  {
    outState.putBoolean(STATE_ALTITUDE_CHART_SHOWN, UiUtils.isVisible(mAltitudeChartFrame));
    if (UiUtils.isVisible(mError))
      outState.putString(STATE_ERROR, mError.getText().toString());
  }

  void restoreRoutingPanelState(@NonNull Bundle state)
  {
    if (state.getBoolean(STATE_ALTITUDE_CHART_SHOWN))
      showAltitudeChartAndRoutingDetails();

    String error = state.getString(STATE_ERROR);
    if (!TextUtils.isEmpty(error))
      showError(error);
  }

  private void showRouteAltitudeChart()
  {
    int chartWidth = UiUtils.dimen(mContext, R.dimen.altitude_chart_image_width);
    int chartHeight = UiUtils.dimen(mContext, R.dimen.altitude_chart_image_height);
    Framework.RouteAltitudeLimits limits = new Framework.RouteAltitudeLimits();
    Bitmap bm = Framework.generateRouteAltitudeChart(chartWidth, chartHeight, limits);
    if (bm != null)
    {
      mAltitudeChart.setImageBitmap(bm);
      UiUtils.show(mAltitudeChart);
      final String unit = limits.isMetricUnits ? mAltitudeDifference.getResources().getString(R.string.meter) : mAltitudeDifference.getResources().getString(R.string.foot);
      mAltitudeDifference.setText(String.format(Locale.getDefault(), "↗ %d %s ↘ %d %s",
                                                limits.totalAscent, unit,
                                                limits.totalDescent, unit));
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

    boolean isHelicopter = RoutingController.get().isHelicopterRouterType();
    Spanned spanned = makeSpannedRoutingDetails(mContext, rinfo, !isHelicopter);
    TextView numbersTime = mNumbersFrame.findViewById(R.id.time);
    numbersTime.setText(spanned);

    TextView numbersArrival = mNumbersFrame.findViewById(R.id.arrival);
    if (numbersArrival != null)
    {
      String arrivalTime = RoutingController.formatArrivalTime(rinfo.totalTimeInSeconds);
      numbersArrival.setText(arrivalTime);
    }
  }

  @NonNull
  private static Spanned makeSpannedRoutingDetails(@NonNull Context context,
                                                   @NonNull RoutingInfo routingInfo,
                                                   boolean showTime)

  {
    CharSequence time = RoutingController.formatRoutingTime(context,
                                                            routingInfo.totalTimeInSeconds,
                                                            R.dimen.text_size_routing_number);

    SpannableStringBuilder builder = new SpannableStringBuilder();

    if (showTime)
    {
      initTimeBuilderSequence(context, time, builder);
      final String dot = "\u00A0• ";
      initDotBuilderSequence(context, dot, builder);
    }

    String dist = routingInfo.distToTarget + "\u00A0" + routingInfo.targetUnits;
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
    final int id = v.getId();
    if (id == R.id.btn__my_position_use && mListener != null)
      mListener.onUseMyPositionAsStart();
    else if (id == R.id.btn__search_point && mListener != null)
    {
      @RoutePointInfo.RouteMarkType
      int pointType = (Integer) mActionMessage.getTag();
      mListener.onSearchRoutePoint(pointType);
    }
  }
}
