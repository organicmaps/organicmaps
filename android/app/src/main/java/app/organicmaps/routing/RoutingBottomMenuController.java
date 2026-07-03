package app.organicmaps.routing;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
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
import android.widget.ScrollView;
import android.widget.TextView;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.DistanceAndAzimut;
import app.organicmaps.sdk.routing.RouteAltitudeData;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.routing.TransitRouteInfo;
import app.organicmaps.sdk.routing.TransitStepInfo;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.RoutingProgressButton;
import app.organicmaps.widget.recycler.DotDividerItemDecoration;
import app.organicmaps.widget.recycler.MultilineLayoutManager;
import java.util.LinkedList;
import java.util.List;

final class RoutingBottomMenuController
{
  enum StartState
  {
    ENABLED,
    DISABLED,
    BUILDING
  }

  private static final String STATE_ALTITUDE_CHART_SHOWN = "altitude_chart_shown";
  private static final String STATE_ERROR = "error";
  // Dimming applied to the save button once the route has been saved (it stays disabled until rebuilt).
  private static final float SAVE_BUTTON_DISABLED_ALPHA = 0.5f;

  @NonNull
  private final Activity mContext;
  @NonNull
  private final View mTimeElevationLine;
  @NonNull
  private final View mAltitudeChartFrame;
  @NonNull
  private final View mTransitTime;
  @NonNull
  private final TextView mError;
  @NonNull
  private final RoutingProgressButton mStart;
  @NonNull
  private final View mAltitudeChart;
  @NonNull
  private final RouteElevationChartController mRouteElevationChartController;
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
  private final DotDividerItemDecoration mTransitViewDecorator;
  @NonNull
  private final TransitStepAdapter mTransitAdapter;
  @NonNull
  private final RecyclerView mTransitRecyclerView;
  @NonNull
  private final View mSaveButton;
  @Nullable
  private final View mManageRoutePanel;
  @Nullable
  private ManageRouteController mManageRouteController;

  @NonNull
  private final RoutingBottomMenuListener mListener;
  @Nullable
  private Runnable mVisibilityChangedCallback;
  @NonNull
  private StartState mStartState = StartState.DISABLED;

  @NonNull
  static RoutingBottomMenuController newInstance(@NonNull Activity activity, @NonNull View frame,
                                                 @NonNull View chartPanel,
                                                 @Nullable RecyclerView.Adapter<?> headerAdapter,
                                                 @NonNull RoutingBottomMenuListener listener)
  {
    // Chart-related ids live inside the chartPanel view tree (it is inflated standalone and hosted by the
    // route-list ConcatAdapter header), so they must be resolved from chartPanel rather than from frame.
    View timeElevationLine = chartPanel.findViewById(R.id.time_elevation_line);
    View transitTime = chartPanel.findViewById(R.id.transit_time);
    TextView rulerTime = chartPanel.findViewById(R.id.time_ruler);
    TextView error = (TextView) getViewById(activity, frame, R.id.error);
    RoutingProgressButton start = (RoutingProgressButton) getViewById(activity, frame, R.id.start);
    View altitudeChart = chartPanel.findViewById(R.id.altitude_chart);
    TextView time = chartPanel.findViewById(R.id.time);
    TextView timeVehicle = chartPanel.findViewById(R.id.time_vehicle);
    TextView altitudeDifference = chartPanel.findViewById(R.id.altitude_difference);
    TextView arrival = (TextView) getViewById(activity, frame, R.id.arrival);
    View saveButton = getViewById(activity, frame, R.id.btn__save);
    return new RoutingBottomMenuController(activity, chartPanel, timeElevationLine, transitTime, rulerTime, error,
                                           start, altitudeChart, time, altitudeDifference, timeVehicle, arrival,
                                           saveButton, headerAdapter, listener);
  }

  @NonNull
  private static View getViewById(@NonNull Activity activity, @NonNull View frame, @IdRes int resourceId)
  {
    View view = frame.findViewById(resourceId);
    return view == null ? activity.findViewById(resourceId) : view;
  }

  private RoutingBottomMenuController(@NonNull Activity context, @NonNull View altitudeChartFrame,
                                      @NonNull View timeElevationLine, @NonNull View transitTime,
                                      @NonNull TextView rulerTime, @NonNull TextView error,
                                      @NonNull RoutingProgressButton start, @NonNull View altitudeChart,
                                      @NonNull TextView time, @NonNull TextView altitudeDifference,
                                      @NonNull TextView timeVehicle, @Nullable TextView arrival,
                                      @NonNull View saveButton, @Nullable RecyclerView.Adapter<?> headerAdapter,
                                      @NonNull RoutingBottomMenuListener listener)
  {
    mContext = context;
    mAltitudeChartFrame = altitudeChartFrame;
    mTimeElevationLine = timeElevationLine;
    mTransitTime = transitTime;
    mTimeRuler = rulerTime;
    mError = error;
    mStart = start;
    mAltitudeChart = altitudeChart;
    mRouteElevationChartController = new RouteElevationChartController(mAltitudeChart);
    mRouteElevationChartController.setListener(new RouteElevationChartController.ElevationSelectionListener() {
      @Override
      public void onElevationPointSelected(double distanceMeters)
      {
        Framework.nativeRouteSetElevationActivePoint(distanceMeters);
      }

      @Override
      public void onElevationPointDeselected()
      {
        Framework.nativeRouteRemoveElevationActivePoint();
      }
    });
    mTime = time;
    mAltitudeDifference = altitudeDifference;
    mTimeVehicle = timeVehicle;
    mArrival = arrival;
    mSaveButton = saveButton;
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
          new ManageRouteController(mManageRoutePanel, headerAdapter, new ManageRouteController.ManageRouteCallback() {
            @Override
            public void onAddStop()
            {
              final RoutingController controller = RoutingController.get();
              if (controller.getStartPoint() == null || controller.getEndPoint() == null)
                return;
              controller.waitForPoiPick(RouteMarkType.Intermediate);
              openSearchForRoutePick();
            }
            @Override
            public void onOpenRouteSearch()
            {
              openSearchForRoutePick();
            }
          });
    }
    mSaveButton.setOnClickListener(v -> saveRoute());
    mStart.setEnabled(false);
    mStart.setOnClickListener(v -> {
      // Ignore the event if the back and start buttons are pressed at the same time.
      // https://github.com/organicmaps/organicmaps/issues/6628
      if (RoutingController.get().isPlanning())
        mListener.onRoutingStart();
    });
    mTransitRecyclerView = altitudeChartFrame.findViewById(R.id.transit_recycler_view);
    mTransitAdapter = new TransitStepAdapter();
    mTransitRecyclerView.setLayoutManager(new MultilineLayoutManager(mAltitudeChartFrame.getLayoutDirection()));
    mTransitRecyclerView.setNestedScrollingEnabled(false);
    mTransitRecyclerView.addItemDecoration(mTransitViewDecorator);
    mTransitRecyclerView.setAdapter(mTransitAdapter);
  }

  private void openSearchForRoutePick()
  {
    ((MwmActivity) mContext).showSearch("");
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
    UiUtils.hide(mError, mAltitudeChart, mTimeElevationLine);

    if (!RoutingController.get().isVehicleRouterType() && !RoutingController.get().isRulerRouterType())
      showRouteAltitudeChart();
    showRoutingDetails();
    UiUtils.show(mAltitudeChartFrame);
    updateSaveButton();
    notifyVisibilityChanged();
    refreshManageRoute();
  }

  // Reflect the current route's saved state on the save button, consistently across every route-details view.
  private void updateSaveButton()
  {
    setSaveButtonEnabled(!RoutingController.get().isRouteSaved());
  }

  // Keeps the enabled flag and the dimming in sync so a saved (disabled) button is always restored on rebuild.
  private void setSaveButtonEnabled(boolean enabled)
  {
    mSaveButton.setEnabled(enabled);
    mSaveButton.setAlpha(enabled ? 1.0f : SAVE_BUTTON_DISABLED_ALPHA);
  }

  void refreshManageRoute()
  {
    if (mManageRouteController != null)
      mManageRouteController.refresh();
  }

  void hideAltitudeChartAndRoutingDetails()
  {
    UiUtils.hide(mAltitudeChart, mTimeVehicle, mTimeElevationLine, mTransitTime, mTimeRuler, mTransitRecyclerView);
    notifyVisibilityChanged();
  }

  @SuppressLint("SetTextI18n")
  void showTransitInfo(@NonNull TransitRouteInfo info)
  {
    refreshManageRoute();
    updateSaveButton();
    View transit_time = mAltitudeChartFrame.findViewById(R.id.transit_time);
    hideAltitudeChartAndRoutingDetails();
    UiUtils.hide(mError, mTimeElevationLine, mTimeVehicle);
    setStartState(StartState.DISABLED);
    UiUtils.show(transit_time, mTransitRecyclerView);
    mTransitAdapter.setItems(info.getTransitSteps());

    scrollToBottom(mTransitRecyclerView);

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
    refreshManageRoute();
    updateSaveButton();
    UiUtils.hide(mError, mTimeVehicle, mTransitTime, mTimeElevationLine, mAltitudeChart);
    setStartState(StartState.DISABLED);
    hideAltitudeChartAndRoutingDetails();
    UiUtils.show(mAltitudeChartFrame, mTransitRecyclerView, mTimeRuler);
    if (points.length > 2)
    {
      UiUtils.show(mTransitRecyclerView);
      mTransitAdapter.setItems(pointsToRulerSteps(points));

      scrollToBottom(mTransitRecyclerView);
    }
    else
      UiUtils.hide(mTransitRecyclerView); // Show only distance between start and finish
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

  // Single entry point for the Start button visual state. The button itself is always visible.
  void setStartState(@NonNull StartState state)
  {
    if (state == mStartState)
      return;
    mStart.setBuildProgress(0);
    mStartState = state;
    mStart.setEnabled(state == StartState.ENABLED);
    notifyVisibilityChanged();
  }

  void setBuildProgress(int progress)
  {
    if (progress > mStart.getBuildProgress())
      mStart.setBuildProgress(progress);
  }

  // Explicit reset covers the rebuild-during-build case where setStartState(BUILDING) early-returns
  // (state unchanged) and would not zero the fill on its own.
  void resetBuildProgress()
  {
    mStart.setBuildProgress(0);
  }

  private void showError(@NonNull String message)
  {
    UiUtils.hide(mAltitudeChartFrame);
    mError.setText(message);
    mError.setVisibility(View.VISIBLE);
    setStartState(StartState.DISABLED);
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
      else if (RoutingController.get().isBuilt())
      {
        setStartState(StartState.ENABLED);
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
    final RouteAltitudeData data = Framework.nativeGetRouteAltitudeData();
    if (data == null || data.getSize() == 0)
    {
      mRouteElevationChartController.clearSelection();
      Framework.nativeRouteRemoveElevationActivePoint();
      mRouteElevationChartController.setData(null);
      UiUtils.hide(mAltitudeChart, mTimeElevationLine);
      return;
    }
    mRouteElevationChartController.setData(data);
    final int formatResId =
        StringUtils.isRtl() ? R.string.route_ascent_descent_format_rtl : R.string.route_ascent_descent_format_ltr;
    final String ascent = Framework.nativeFormatAltitude(data.getTotalAscent());
    final String descent = Framework.nativeFormatAltitude(data.getTotalDescent());
    mAltitudeDifference.setText(mContext.getString(formatResId, ascent, descent));
    UiUtils.show(mAltitudeDifference, mAltitudeChart);
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

  private void saveRoute()
  {
    if (!RoutingController.get().isBuilt())
      return;
    // Clear elevation preview marker before saving.
    mRouteElevationChartController.clearSelection();
    Framework.nativeRouteRemoveElevationActivePoint();
    Framework.nativeSaveRoute();
    RoutingController.get().setRouteSaved();
    setSaveButtonEnabled(false);
  }
}
