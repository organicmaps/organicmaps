package app.organicmaps.routing;

import static app.organicmaps.sdk.util.Utils.dimen;

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
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ScrollView;
import android.widget.TextView;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.DistanceAndAzimut;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.routing.TransitRouteInfo;
import app.organicmaps.sdk.routing.TransitStepInfo;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.search.SearchActivity;
import app.organicmaps.sdk.util.Graphics;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.recycler.DotDividerItemDecoration;
import app.organicmaps.widget.recycler.MultilineLayoutManager;
import java.util.LinkedList;
import java.util.List;

final class RoutingBottomMenuController implements View.OnClickListener
{
  private static final String STATE_ALTITUDE_CHART_SHOWN = "altitude_chart_shown";
  private static final String STATE_ERROR = "error";

  @NonNull
  private final Activity mContext;
  @NonNull
  private final View mTimeElevationLine;
  @NonNull
  private final View mAltitudeChartFrame;
  @NonNull
  private final View mTransitTime;
  @NonNull
  private final View mTransitRecylerView;
  @NonNull
  private final TextView mError;
  @NonNull
  private final Button mStart;
  @NonNull
  private final ImageView mAltitudeChart;
  @NonNull
  private final TextView mTime;
  @NonNull
  private final TextView mAltitudeDifference;
  @NonNull
  private final TextView mTimeVehicle;
  @NonNull
  private final TextView mTimeRuler;

  @Nullable
  private final TextView mArrival;
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
  @NonNull
  private final View mSaveButton;
  @Nullable
  private final View mManageRoutePanel;
  @Nullable
  private ManageRouteController mManageRouteController;

  @Nullable
  private final RoutingBottomMenuListener mListener;
  @Nullable
  private Runnable mVisibilityChangedCallback;

  @NonNull
  static RoutingBottomMenuController newInstance(@NonNull Activity activity, @NonNull View frame,
                                                 @NonNull RoutingBottomMenuListener listener)
  {
    View altitudeChartFrame = getViewById(activity, frame, R.id.altitude_chart_panel);
    View timeElevationLine = getViewById(activity, frame, R.id.time_elevation_line);
    View transitTime = getViewById(activity, frame, R.id.transit_time);
    View transitRecyclerView = getViewById(activity, frame, R.id.transit_recycler_view);
    TextView rulerTime = (TextView) getViewById(activity, frame, R.id.time_ruler);
    TextView error = (TextView) getViewById(activity, frame, R.id.error);
    Button start = (Button) getViewById(activity, frame, R.id.start);
    ImageView altitudeChart = (ImageView) getViewById(activity, frame, R.id.altitude_chart);
    TextView time = (TextView) getViewById(activity, frame, R.id.time);
    TextView timeVehicle = (TextView) getViewById(activity, frame, R.id.time_vehicle);
    TextView altitudeDifference = (TextView) getViewById(activity, frame, R.id.altitude_difference);
    TextView arrival = (TextView) getViewById(activity, frame, R.id.arrival);
    View actionFrame = getViewById(activity, frame, R.id.routing_action_frame);
    View saveButton = getViewById(activity, frame, R.id.btn__save);
    return new RoutingBottomMenuController(activity, altitudeChartFrame, timeElevationLine, transitTime, rulerTime,
                                           transitRecyclerView, error, start, altitudeChart, time, altitudeDifference,
                                           timeVehicle, arrival, actionFrame, saveButton, listener);
  }

  @NonNull
  private static View getViewById(@NonNull Activity activity, @NonNull View frame, @IdRes int resourceId)
  {
    View view = frame.findViewById(resourceId);
    if (view == null && frame.getParent() instanceof View)
      view = ((View) frame.getParent()).findViewById(resourceId);
    return view == null ? activity.findViewById(resourceId) : view;
  }

  private RoutingBottomMenuController(@NonNull Activity context, @NonNull View altitudeChartFrame,
                                      @NonNull View timeElevationLine, @NonNull View transitTime,
                                      @NonNull TextView rulerTime, @NonNull View transitRecyclerView,
                                      @NonNull TextView error, @NonNull Button start, @NonNull ImageView altitudeChart,
                                      @NonNull TextView time, @NonNull TextView altitudeDifference,
                                      @NonNull TextView timeVehicle, @Nullable TextView arrival,
                                      @NonNull View actionFrame, @NonNull View saveButton,
                                      @Nullable RoutingBottomMenuListener listener)
  {
    mContext = context;

    mAltitudeChartFrame = altitudeChartFrame;
    mTimeElevationLine = timeElevationLine;
    mTransitTime = transitTime;
    mTimeRuler = rulerTime;
    mTransitRecylerView = transitRecyclerView;
    mError = error;
    mStart = start;
    mAltitudeChart = altitudeChart;
    mTime = time;
    mAltitudeDifference = altitudeDifference;
    mTimeVehicle = timeVehicle;
    mArrival = arrival;
    mActionFrame = actionFrame;
    mActionMessage = actionFrame.findViewById(R.id.tv__message);
    mActionButton = actionFrame.findViewById(R.id.btn__my_position_use);
    mSaveButton = saveButton;
    mActionButton.setOnClickListener(this);
    View actionSearchButton = actionFrame.findViewById(R.id.btn__search_point);
    actionSearchButton.setOnClickListener(this);
    mActionIcon = mActionButton.findViewById(R.id.iv__icon);
    UiUtils.hide(mActionFrame);
    mListener = listener;
    int dividerRes = ThemeUtils.getResource(mContext, R.attr.transitStepDivider);
    Drawable dividerDrawable = ContextCompat.getDrawable(mContext, dividerRes);
    Resources res = mContext.getResources();
    mTransitViewDecorator =
        new DotDividerItemDecoration(dividerDrawable, res.getDimensionPixelSize(R.dimen.margin_base),
                                     res.getDimensionPixelSize(R.dimen.margin_half));
    mManageRoutePanel = mContext.findViewById(R.id.manage_route_panel);
    if (mManageRoutePanel != null)
    {
      mManageRouteController =
          new ManageRouteController(mManageRoutePanel, new ManageRouteController.ManageRouteCallback() {
            public void onAddStop()
            {
              RoutingController.get().waitForPoiPick(RouteMarkType.Intermediate);
              SearchActivity.start(mContext, "");
            }
            @Override
            public void onReplaceStop()
            {
              SearchActivity.start(mContext, "");
            }
          });
    }
    mSaveButton.setOnClickListener(this);
  }

  void setVisibilityChangedCallback(@Nullable Runnable callback)
  {
    mVisibilityChangedCallback = callback;
  }

  private void notifyVisibilityChanged()
  {
    if (mVisibilityChangedCallback != null)
      mVisibilityChangedCallback.run();
  }

  void showAltitudeChartAndRoutingDetails()
  {
    UiUtils.hide(mError, mActionFrame, mAltitudeChart, mTimeElevationLine, mTransitTime, mTransitRecylerView);
    refreshManageRoute();
    if (!RoutingController.get().isVehicleRouterType() && !RoutingController.get().isRulerRouterType())
      showRouteAltitudeChart();
    showRoutingDetails();
    UiUtils.show(mAltitudeChartFrame);
    mSaveButton.setEnabled(true);
    notifyVisibilityChanged();
  }

  void refreshManageRoute()
  {
    if (mManageRouteController != null)
      mManageRouteController.refresh();
  }

  void hideAltitudeChartAndRoutingDetails()
  {
    UiUtils.hide(mAltitudeChart, mTimeVehicle, mTimeElevationLine, mTransitTime, mTimeRuler, mTransitRecylerView,
                 mActionFrame);
    notifyVisibilityChanged();
  }

  @SuppressLint("SetTextI18n")
  void showTransitInfo(@NonNull TransitRouteInfo info)
  {
    refreshManageRoute();
    View transit_time = mAltitudeChartFrame.findViewById(R.id.transit_time);
    hideAltitudeChartAndRoutingDetails();
    UiUtils.hide(mError, mActionFrame, mTimeElevationLine, mTimeVehicle);
    showStartButton(false);
    UiUtils.show(transit_time, mTransitRecylerView);
    RecyclerView rv = mAltitudeChartFrame.findViewById(R.id.transit_recycler_view);
    TransitStepAdapter adapter = new TransitStepAdapter();
    rv.setLayoutManager(new MultilineLayoutManager(mAltitudeChartFrame.getLayoutDirection()));
    rv.setNestedScrollingEnabled(false);
    rv.removeItemDecoration(mTransitViewDecorator);
    rv.addItemDecoration(mTransitViewDecorator);
    rv.setAdapter(adapter);
    adapter.setItems(info.getTransitSteps());

    scrollToBottom(rv);

    TextView totalTimeView = mAltitudeChartFrame.findViewById(R.id.total_time);
    totalTimeView.setText(Utils.formatRoutingTime(mContext, info.getTotalTime(), R.dimen.text_size_routing_number));
    View dotView = mAltitudeChartFrame.findViewById(R.id.dot);
    View pedestrianIcon = mAltitudeChartFrame.findViewById(R.id.pedestrian_icon);
    TextView distanceView = mAltitudeChartFrame.findViewById(R.id.total_distance);
    UiUtils.showIf(info.getTotalPedestrianTimeInSec() > 0, dotView, pedestrianIcon, distanceView);
    distanceView.setText(info.getTotalPedestrianDistance() + " " + info.getTotalPedestrianDistanceUnits());
    notifyVisibilityChanged();
  }

  @SuppressLint("SetTextI18n")
  void showRulerInfo(@NonNull RouteMarkData[] points, Distance totalLength)
  {
    UiUtils.hide(mError, mActionFrame, mTimeVehicle, mTransitTime, mTimeElevationLine, mAltitudeChart);
    refreshManageRoute();
    showStartButton(false);
    hideAltitudeChartAndRoutingDetails();
    UiUtils.show(mAltitudeChartFrame, mTransitRecylerView, mTimeRuler);
    final RecyclerView rv = mAltitudeChartFrame.findViewById(R.id.transit_recycler_view);
    if (points.length > 2)
    {
      UiUtils.show(rv);
      final TransitStepAdapter adapter = new TransitStepAdapter();
      rv.setLayoutManager(new MultilineLayoutManager(mAltitudeChartFrame.getLayoutDirection()));
      rv.setNestedScrollingEnabled(false);
      rv.removeItemDecoration(mTransitViewDecorator);
      rv.addItemDecoration(mTransitViewDecorator);
      rv.setAdapter(adapter);
      adapter.setItems(pointsToRulerSteps(points));

      scrollToBottom(rv);
    }
    else
      UiUtils.hide(rv); // Show only distance between start and finish
    mTimeRuler.setText(mContext.getString(R.string.placepage_distance) + ": " + totalLength.mDistanceStr + " "
                       + totalLength.getUnitsStr(mContext));
    notifyVisibilityChanged();
  }

  // Create steps info to use in TransitStepAdapter.
  private List<TransitStepInfo> pointsToRulerSteps(RouteMarkData[] points)
  {
    List<TransitStepInfo> transitSteps = new LinkedList<>();
    for (int i = 1; i < points.length; i++)
    {
      RouteMarkData segmentStart = points[i - 1];
      RouteMarkData segmentEnd = points[i];
      DistanceAndAzimut dist = Framework.nativeGetDistanceAndAzimuthFromLatLon(segmentStart.mLat, segmentStart.mLon,
                                                                               segmentEnd.mLat, segmentEnd.mLon, 0);
      if (i > 1)
        transitSteps.add(TransitStepInfo.intermediatePoint(i - 2));
      transitSteps.add(
          TransitStepInfo.ruler(dist.getDistance().mDistanceStr, dist.getDistance().getUnitsStr(mContext)));
    }

    return transitSteps;
  }

  void showAddStartFrame()
  {
    UiUtils.hide(mError, mTransitTime);
    UiUtils.show(mActionFrame);
    mActionMessage.setText(R.string.routing_add_start_point);
    mActionMessage.setTag(RouteMarkType.Start);
    if (MwmApplication.from(mContext).getLocationHelper().getMyPosition() != null)
    {
      UiUtils.show(mActionButton);
      Drawable icon = ContextCompat.getDrawable(mContext, R.drawable.ic_location_crosshair);
      int colorAccent = ContextCompat.getColor(
          mContext, UiUtils.getStyledResourceId(mContext, androidx.appcompat.R.attr.colorAccent));
      mActionIcon.setImageDrawable(Graphics.tint(icon, colorAccent));
    }
    else
    {
      UiUtils.hide(mActionButton);
    }
    notifyVisibilityChanged();
  }

  void showAddFinishFrame()
  {
    UiUtils.hide(mError, mTransitTime);
    UiUtils.show(mActionFrame);
    mActionMessage.setText(R.string.routing_add_finish_point);
    mActionMessage.setTag(RouteMarkType.Finish);
    UiUtils.hide(mActionButton);
    notifyVisibilityChanged();
  }

  void hideActionFrame()
  {
    UiUtils.hide(mActionFrame);
    notifyVisibilityChanged();
  }

  void setStartButton(boolean show)
  {
    if (show)
    {
      mStart.setText(mContext.getText(R.string.p2p_start));
      mStart.setOnClickListener(v -> {
        // Ignore the event if the back and start buttons are pressed at the same time.
        // See {@link #RoutingPlanController.onUpClick()}.
        // https://github.com/organicmaps/organicmaps/issues/6628
        if (mListener != null && RoutingController.get().isPlanning())
          mListener.onRoutingStart();
      });
    }

    showStartButton(show);
  }

  private void showError(@NonNull String message)
  {
    UiUtils.hide(mAltitudeChartFrame, mActionFrame);
    mError.setText(message);
    mError.setVisibility(View.VISIBLE);
    showStartButton(false);
    notifyVisibilityChanged();
  }

  void showStartButton(boolean show)
  {
    boolean result = show && RoutingController.get().isBuilt();
    UiUtils.showIf(result, mStart);
    notifyVisibilityChanged();
  }

  void saveRoutingPanelState(@NonNull Bundle outState)
  {
    outState.putBoolean(STATE_ALTITUDE_CHART_SHOWN, UiUtils.isVisible(mAltitudeChartFrame));
    if (UiUtils.isVisible(mError))
      outState.putString(STATE_ERROR, mError.getText().toString());
  }

  void restoreRoutingPanelState(@NonNull Bundle state)
  {
    Log.d("stateroute", String.valueOf(state.getBoolean(STATE_ALTITUDE_CHART_SHOWN)));
    if (state.getBoolean(STATE_ALTITUDE_CHART_SHOWN))
    {
      if (RoutingController.get().isTransitType())
      {
        TransitRouteInfo info = RoutingController.get().getCachedTransitInfo();
        if (info != null)
          showTransitInfo(info);
      }
      else if (RoutingController.get().isRulerRouterType())
      {
        RoutingInfo routingInfo = RoutingController.get().getCachedRoutingInfo();
        if (routingInfo != null)
          showRulerInfo(Framework.nativeGetRoutePoints(), routingInfo.distToTarget);
      }
      else
      {
        setStartButton(true);
        showAltitudeChartAndRoutingDetails();
      }
    }

    String error = state.getString(STATE_ERROR);
    if (!TextUtils.isEmpty(error))
      showError(error);
  }

  private void showRouteAltitudeChart()
  {
    hideAltitudeChartAndRoutingDetails();
    if (RoutingController.get().isVehicleRouterType())
    {
      UiUtils.hide(mTimeElevationLine, mAltitudeChart);
      return;
    }

    UiUtils.hide(mTimeVehicle);
    final View altitude_chart_container = mAltitudeChartFrame.findViewById(R.id.altitude_chart_container);

    int chartWidth = altitude_chart_container.getWidth();
    if (chartWidth == 0)
      return;
    int chartHeight = dimen(mContext, R.dimen.altitude_chart_image_height);
    Framework.RouteAltitudeLimits limits = new Framework.RouteAltitudeLimits();
    Bitmap bm = Framework.generateRouteAltitudeChart(chartWidth, chartHeight, limits);
    if (bm != null)
    {
      mAltitudeChart.setImageBitmap(bm);
      UiUtils.show(mAltitudeChart);
      final String unit = limits.isMetricUnits
                            ? mAltitudeDifference.getResources().getString(app.organicmaps.sdk.R.string.m)
                            : mAltitudeDifference.getResources().getString(app.organicmaps.sdk.R.string.ft);
      mAltitudeDifference.setText("↗ " + limits.totalAscentString + " " + unit + " ↘ " + limits.totalDescentString + " "
                                  + unit);
      UiUtils.show(mAltitudeDifference, mAltitudeChart);
    }
  }

  private void showRoutingDetails()
  {
    final RoutingInfo rinfo = RoutingController.get().getCachedRoutingInfo();
    if (rinfo == null)
    {
      UiUtils.hide(mTimeElevationLine, mTimeVehicle);
      return;
    }

    Spanned spanned = makeSpannedRoutingDetails(mContext, rinfo);
    if (RoutingController.get().isVehicleRouterType())
    {
      hideAltitudeChartAndRoutingDetails();
      UiUtils.show(mTimeVehicle);
      mTimeVehicle.setText(spanned);
    }
    else
    {
      UiUtils.show(mTimeElevationLine);
      mTime.setText(spanned);
    }

    if (mArrival != null)
    {
      String arrivalTime = Utils.formatArrivalTime(rinfo.totalTimeInSeconds);
      mArrival.setText(arrivalTime);
    }
  }

  // Scroll RecyclerView to bottom using parent ScrollView.
  private static void scrollToBottom(RecyclerView rv)
  {
    final ScrollView parentScroll = (ScrollView) rv.getParent();
    if (parentScroll != null)
      parentScroll.postDelayed(() -> parentScroll.fullScroll(ScrollView.FOCUS_DOWN), 100);
  }

  @NonNull
  private static Spanned makeSpannedRoutingDetails(@NonNull Context context, @NonNull RoutingInfo routingInfo)

  {
    CharSequence time =
        Utils.formatRoutingTime(context, routingInfo.totalTimeInSeconds, R.dimen.text_size_routing_number);

    SpannableStringBuilder builder = new SpannableStringBuilder();
    initTimeBuilderSequence(context, time, builder);

    String dot = "\u00A0• ";
    initDotBuilderSequence(context, dot, builder);

    initDistanceBuilderSequence(context, routingInfo.distToTarget.toString(context), builder);

    return builder;
  }

  private static void initTimeBuilderSequence(@NonNull Context context, @NonNull CharSequence time,
                                              @NonNull SpannableStringBuilder builder)
  {
    builder.append(time);

    builder.setSpan(new TypefaceSpan(context.getResources().getString(R.string.robotoMedium)), 0, builder.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(
        new AbsoluteSizeSpan(context.getResources().getDimensionPixelSize(R.dimen.text_size_routing_number)), 0,
        builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new StyleSpan(Typeface.BOLD), 0, builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new ForegroundColorSpan(ThemeUtils.getColor(context, android.R.attr.textColorPrimary)), 0,
                    builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
  }

  private static void initDotBuilderSequence(@NonNull Context context, @NonNull String dot,
                                             @NonNull SpannableStringBuilder builder)
  {
    builder.append(dot);
    builder.setSpan(new TypefaceSpan(context.getResources().getString(R.string.robotoMedium)),
                    builder.length() - dot.length(), builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(
        new AbsoluteSizeSpan(context.getResources().getDimensionPixelSize(R.dimen.text_size_routing_number)),
        builder.length() - dot.length(), builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new ForegroundColorSpan(ThemeUtils.getColor(context, R.attr.secondary)),
                    builder.length() - dot.length(), builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
  }

  private static void initDistanceBuilderSequence(@NonNull Context context, @NonNull String arrivalTime,
                                                  @NonNull SpannableStringBuilder builder)
  {
    builder.append(arrivalTime);
    builder.setSpan(new TypefaceSpan(context.getResources().getString(R.string.robotoMedium)),
                    builder.length() - arrivalTime.length(), builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(
        new AbsoluteSizeSpan(context.getResources().getDimensionPixelSize(R.dimen.text_size_routing_number)),
        builder.length() - arrivalTime.length(), builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new StyleSpan(Typeface.NORMAL), builder.length() - arrivalTime.length(), builder.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new ForegroundColorSpan(ThemeUtils.getColor(context, android.R.attr.textColorPrimary)),
                    builder.length() - arrivalTime.length(), builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
  }

  public boolean onBackPressed()
  {
    if (mManageRoutePanel != null && UiUtils.isVisible(mManageRoutePanel))
    {
      UiUtils.hide(mManageRoutePanel);
      return true;
    }
    return false;
  }

  @Override
  public void onClick(View v)
  {
    final int id = v.getId();
    if (id == R.id.btn__my_position_use)
      mListener.onUseMyPositionAsStart();
    else if (id == R.id.btn__search_point)
    {
      final RouteMarkType pointType = (RouteMarkType) mActionMessage.getTag();
      mListener.onSearchRoutePoint(pointType);
    }
    else if (id == R.id.btn__save)
    {
      Framework.nativeSaveRoute();
      View saveButton = v.findViewById(R.id.btn__save);
      saveButton.setEnabled(false);
      saveButton.setAlpha(0.05f);
    }
  }
}
