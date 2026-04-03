package app.organicmaps.widget.placepage;

import android.content.Context;
import android.graphics.Color;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import app.organicmaps.R;
import app.organicmaps.util.ThemeUtils;
import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.XAxis;
import com.github.mikephil.charting.components.YAxis;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.github.mikephil.charting.formatter.ValueFormatter;
import java.util.List;

public final class ElevationChartUtils
{
  public static final int TRACK_X_LABEL_COUNT = 6;

  private static final int CHART_Y_LABEL_COUNT = 3;
  private static final int CHART_FILL_ALPHA = (int) (0.12 * 255);
  private static final int CHART_AXIS_GRANULARITY = 100;
  private static final String ELEVATION_PROFILE_POINTS = "ELEVATION_PROFILE_POINTS";

  private ElevationChartUtils() {}

  public static void setupChart(@NonNull LineChart chart, @NonNull Context context)
  {
    chart.setBackgroundColor(ThemeUtils.getColor(context, R.attr.cardBackground));
    chart.setTouchEnabled(true);
    chart.setHighlightPerDragEnabled(true);
    chart.setHighlighter(new InterpolatingHighlighter(chart));
    chart.setRenderer(new SmoothLineChartRenderer(chart, chart.getAnimator(), chart.getViewPortHandler()));
    chart.setDrawGridBackground(false);
    chart.setScaleXEnabled(true);
    chart.setScaleYEnabled(false);
    chart.setExtraTopOffset(0);
    int sideOffset = context.getResources().getDimensionPixelSize(R.dimen.margin_base);
    chart.setViewPortOffsets(sideOffset, 0, sideOffset,
                             context.getResources().getDimensionPixelSize(R.dimen.margin_base_plus_quarter));
    chart.getDescription().setEnabled(false);
    chart.setDrawBorders(false);
    chart.getLegend().setEnabled(false);
  }

  public static void initAxes(@NonNull LineChart chart, @NonNull Context context, int xLabelCount)
  {
    XAxis x = chart.getXAxis();
    x.setLabelCount(xLabelCount, false);
    x.setDrawGridLines(false);
    x.setGranularity(CHART_AXIS_GRANULARITY);
    x.setGranularityEnabled(true);
    x.setTextColor(ThemeUtils.getColor(context, R.attr.elevationProfileAxisLabelColor));
    x.setPosition(XAxis.XAxisPosition.BOTTOM);
    x.setAxisLineColor(ThemeUtils.getColor(context, androidx.appcompat.R.attr.dividerHorizontal));
    x.setAxisLineWidth(context.getResources().getDimensionPixelSize(R.dimen.divider_height));
    ValueFormatter xAxisFormatter = new AxisValueFormatter(chart);
    x.setValueFormatter(xAxisFormatter);

    YAxis y = chart.getAxisLeft();
    y.setLabelCount(CHART_Y_LABEL_COUNT, false);
    y.setPosition(YAxis.YAxisLabelPosition.INSIDE_CHART);
    y.setDrawGridLines(true);
    y.setGridColor(ContextCompat.getColor(context, R.color.black_12));
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
