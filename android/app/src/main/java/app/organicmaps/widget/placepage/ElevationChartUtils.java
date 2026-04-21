package app.organicmaps.widget.placepage;

import android.content.Context;
import android.graphics.Color;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.settings.UnitLocale;
import app.organicmaps.util.ThemeUtils;
import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.LimitLine;
import com.github.mikephil.charting.components.XAxis;
import com.github.mikephil.charting.components.YAxis;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.github.mikephil.charting.formatter.ValueFormatter;
import java.util.List;

public final class ElevationChartUtils
{
  private static final int TRACK_X_LABEL_COUNT = 6;
  private static final int ROUTE_X_LABEL_COUNT = 3;

  private static final int CHART_Y_LABEL_COUNT = 3;
  private static final int CHART_Y_MAX_STEP_COUNT = 6;
  private static final int CHART_FILL_ALPHA = (int) (0.12 * 255);
  private static final int CHART_AXIS_GRANULARITY = 100;

  private static final String ELEVATION_PROFILE_POINTS = "ELEVATION_PROFILE_POINTS";

  private ElevationChartUtils() {}

  public static void setupTrackChart(@NonNull LineChart chart, @NonNull Context context)
  {
    int topOffset = context.getResources().getDimensionPixelSize(R.dimen.margin_quarter);
    setupCommon(chart, context, topOffset, TRACK_X_LABEL_COUNT);
  }

  public static void setupRouteChart(@NonNull LineChart chart, @NonNull Context context)
  {
    setupCommon(chart, context, 0, ROUTE_X_LABEL_COUNT);
  }

  private static void setupCommon(@NonNull LineChart chart, @NonNull Context context, int topOffset, int xLabelCount)
  {
    chart.setBackgroundColor(ThemeUtils.getColor(context, R.attr.cardBackground));
    chart.setTouchEnabled(true);
    chart.setHighlightPerDragEnabled(false);
    chart.setHighlighter(new InterpolatingHighlighter(chart));
    chart.setRenderer(new SmoothLineChartRenderer(chart, chart.getAnimator(), chart.getViewPortHandler()));
    chart.setDrawGridBackground(false);
    chart.setScaleXEnabled(true);
    chart.setScaleYEnabled(false);
    chart.setExtraTopOffset(0);
    int sideOffset = context.getResources().getDimensionPixelSize(R.dimen.margin_base);
    chart.setViewPortOffsets(sideOffset, topOffset, sideOffset,
                             context.getResources().getDimensionPixelSize(R.dimen.margin_base_plus_quarter));
    chart.getDescription().setEnabled(false);
    chart.setDrawBorders(false);
    chart.getLegend().setEnabled(false);
    initAxes(chart, context, xLabelCount);
  }

  private static void initAxes(@NonNull LineChart chart, @NonNull Context context, int xLabelCount)
  {
    XAxis x = chart.getXAxis();
    x.setLabelCount(xLabelCount, false);
    x.setDrawGridLines(false);
    x.setGranularity(CHART_AXIS_GRANULARITY);
    x.setGranularityEnabled(true);
    x.setTextColor(ThemeUtils.getColor(context, R.attr.elevationProfileAxisLabelColor));
    x.setPosition(XAxis.XAxisPosition.BOTTOM);
    int dividerColor = ThemeUtils.getColor(context, androidx.appcompat.R.attr.dividerHorizontal);
    x.setAxisLineColor(dividerColor);
    x.setAxisLineWidth(context.getResources().getDimensionPixelSize(R.dimen.divider_height));
    ValueFormatter xAxisFormatter = new AxisValueFormatter(chart);
    x.setValueFormatter(xAxisFormatter);
    x.setDrawLimitLinesBehindData(false);

    YAxis y = chart.getAxisLeft();
    y.setLabelCount(CHART_Y_LABEL_COUNT, false);
    y.setPosition(YAxis.YAxisLabelPosition.INSIDE_CHART);
    y.setDrawGridLines(true);
    y.setGridColor(dividerColor);
    y.setEnabled(true);
    y.setTextColor(Color.TRANSPARENT);
    y.setAxisLineColor(Color.TRANSPARENT);
    int lineLength = context.getResources().getDimensionPixelSize(R.dimen.margin_eighth);
    y.enableGridDashedLine(lineLength, 2 * lineLength, 0);

    chart.getAxisRight().setEnabled(false);
  }

  public static void applyLineDataSetStyle(@NonNull LineDataSet set, @NonNull Context context)
  {
    set.setMode(LineDataSet.Mode.LINEAR);
    set.setDrawFilled(true);
    set.setDrawCircles(false);
    int lineThickness = context.getResources().getDimensionPixelSize(R.dimen.divider_width);
    set.setLineWidth(lineThickness);
    int color = ThemeUtils.getColor(context, R.attr.elevationProfileColor);
    set.setCircleColor(color);
    set.setColor(color);
    set.setFillAlpha(CHART_FILL_ALPHA);
    set.setFillColor(color);
    set.setDrawHorizontalHighlightIndicator(false);
    set.setHighlightLineWidth(lineThickness);
    set.setHighLightColor(ContextCompat.getColor(context, R.color.base_accent_transparent));
  }

  public static void setChartData(@NonNull LineChart chart, @NonNull List<Entry> values, @NonNull Context context)
  {
    LineDataSet set = new LineDataSet(values, ELEVATION_PROFILE_POINTS);
    applyLineDataSetStyle(set, context);
    LineData data = new LineData(set);
    data.setDrawValues(false);
    chart.setData(data);
    chart.animateX(0);
    // Reset zoom after layout to handle viewport size changes on rotation.
    chart.post(chart::fitScreen);
  }

  public static void configureYAxisBounds(@NonNull LineChart chart, float minAltitude, float maxAltitude)
  {
    // Step size in meters: 50m for metric, 100ft (30.48m) for imperial.
    boolean isImperial = UnitLocale.getUnits() == UnitLocale.UNITS_FOOT;
    float altitudeStep = isImperial ? 30.48f : 50f;

    float range = maxAltitude - minAltitude;
    float lower;
    float upper;
    if (range < 1f)
    {
      // Flat track: expand to at least one step on each side.
      lower = minAltitude - altitudeStep;
      upper = maxAltitude + altitudeStep;
    }
    else
    {
      float padding = Math.round(range / 10f);
      lower = minAltitude - padding;
      upper = maxAltitude + padding;
    }

    // Round to step boundaries.
    lower = (float) (Math.floor(lower / altitudeStep) * altitudeStep);
    upper = (float) (Math.ceil(upper / altitudeStep) * altitudeStep);

    // Guard against degenerate range after rounding.
    if (upper <= lower)
      upper = lower + altitudeStep;

    // Determine label count: double step size while more than CHART_Y_MAX_STEP_COUNT labels.
    float effectiveStep = altitudeStep;
    int stepCount = (int) Math.ceil((upper - lower) / effectiveStep);
    while (stepCount > CHART_Y_MAX_STEP_COUNT && effectiveStep < upper - lower)
    {
      effectiveStep *= 2;
      stepCount = (int) Math.ceil((upper - lower) / effectiveStep);
    }
    // Ensure range is an exact multiple of effectiveStep so labels land on step boundaries.
    upper = lower + stepCount * effectiveStep;

    YAxis y = chart.getAxisLeft();
    y.setAxisMinimum(lower);
    y.setAxisMaximum(upper);
    y.setLabelCount(stepCount + 1, true);
  }

  public static void addSegmentSeparators(@NonNull LineChart chart, @NonNull double[] segmentDistances,
                                          @NonNull Context context)
  {
    XAxis xAxis = chart.getXAxis();
    xAxis.removeAllLimitLines();

    if (segmentDistances.length == 0)
      return;

    int lineColor = ThemeUtils.getColor(context, androidx.appcompat.R.attr.dividerHorizontal);
    // setLineWidth() takes dp (auto-converts internally), enableDashedLine() takes raw pixels.
    float lineWidth = 2f;
    int dashOn = context.getResources().getDimensionPixelSize(R.dimen.elevation_profile_separator_dash);
    int dashOff = context.getResources().getDimensionPixelSize(R.dimen.elevation_profile_separator_gap);

    for (double distance : segmentDistances)
    {
      LimitLine line = new LimitLine((float) distance);
      line.setLineColor(lineColor);
      line.setLineWidth(lineWidth);
      line.enableDashedLine(dashOn, dashOff, 0f);
      line.setLabel("");
      xAxis.addLimitLine(line);
    }
  }

  public static float interpolateY(@NonNull List<Entry> entries, float xVal)
  {
    if (entries.isEmpty())
      return 0f;

    if (xVal <= entries.get(0).getX())
      return entries.get(0).getY();

    for (int i = 1; i < entries.size(); i++)
    {
      Entry cur = entries.get(i);
      if (xVal <= cur.getX())
      {
        Entry prev = entries.get(i - 1);
        float segLen = cur.getX() - prev.getX();
        if (segLen < 1e-9f)
          return prev.getY();
        float fraction = (xVal - prev.getX()) / segLen;
        return prev.getY() + fraction * (cur.getY() - prev.getY());
      }
    }

    return entries.get(entries.size() - 1).getY();
  }
}
