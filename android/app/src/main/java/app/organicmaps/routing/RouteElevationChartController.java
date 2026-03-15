package app.organicmaps.routing;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Color;
import android.view.View;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.routing.RouteAltitudeData;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.widget.placepage.AxisValueFormatter;
import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.Legend;
import com.github.mikephil.charting.components.XAxis;
import com.github.mikephil.charting.components.YAxis;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.github.mikephil.charting.formatter.ValueFormatter;
import com.github.mikephil.charting.highlight.Highlight;
import com.github.mikephil.charting.listener.OnChartValueSelectedListener;
import java.util.ArrayList;
import java.util.List;

public class RouteElevationChartController
{
  private static final int CHART_Y_LABEL_COUNT = 3;
  private static final int CHART_X_LABEL_COUNT = 6;
  private static final int CHART_ANIMATION_DURATION = 1000;
  private static final int CHART_FILL_ALPHA = (int) (0.12 * 255);
  private static final int CHART_AXIS_GRANULARITY = 100;
  private static final String ELEVATION_PROFILE_POINTS = "ELEVATION_PROFILE_POINTS";

  @NonNull
  private final Context mContext;
  @NonNull
  private final LineChart mChart;
  @NonNull
  private final TextView mMaxAltitude;
  @NonNull
  private final TextView mMinAltitude;

  @Nullable
  private ElevationSelectionListener mListener;
  @Nullable
  private RouteAltitudeData mData;

  public interface ElevationSelectionListener
  {
    void onElevationPointSelected(double lat, double lon);
    void onElevationPointDeselected();
  }

  public RouteElevationChartController(@NonNull View view)
  {
    mContext = view.getContext();
    final Resources resources = mContext.getResources();
    // Assuming the layout will have these IDs, same as bookmark chart for now
    mChart = view.findViewById(R.id.elevation_profile_chart);
    mMaxAltitude = view.findViewById(R.id.highest_altitude);
    mMinAltitude = view.findViewById(R.id.lowest_altitude);

    setupChart(resources);
    mChart.setOnChartValueSelectedListener(new OnChartValueSelectedListener() {
      @Override
      public void onValueSelected(Entry e, Highlight h)
      {
        if (mListener != null && mData != null)
        {
          Object dataObj = e.getData();
          if (dataObj instanceof Integer)
          {
            int index = (Integer) dataObj;
            if (index >= 0 && index < mData.lats.length)
            {
              mListener.onElevationPointSelected(mData.lats[index], mData.lons[index]);
            }
          }
        }
      }

      @Override
      public void onNothingSelected()
      {
        if (mListener != null)
        {
          mListener.onElevationPointDeselected();
        }
      }
    });
  }

  private void setupChart(@NonNull Resources resources)
  {
    mChart.setBackgroundColor(ThemeUtils.getColor(mContext, R.attr.cardBackground));
    mChart.setTouchEnabled(true);
    mChart.setDragEnabled(true);
    mChart.setScaleEnabled(true);
    mChart.setDrawGridBackground(false);
    mChart.setScaleXEnabled(true);
    mChart.setScaleYEnabled(false);
    mChart.setExtraTopOffset(0);
    int sideOffset = resources.getDimensionPixelSize(R.dimen.margin_base);
    int topOffset = 0;
    mChart.setViewPortOffsets(sideOffset, topOffset, sideOffset,
                              resources.getDimensionPixelSize(R.dimen.margin_base_plus_quarter));
    mChart.getDescription().setEnabled(false);
    mChart.setDrawBorders(false);
    Legend l = mChart.getLegend();
    l.setEnabled(false);
    initAxes();
  }

  public void setListener(@Nullable ElevationSelectionListener listener)
  {
    mListener = listener;
  }

  private void initAxes()
  {
    XAxis x = mChart.getXAxis();
    x.setLabelCount(CHART_X_LABEL_COUNT, false);
    x.setDrawGridLines(false);
    x.setGranularity(CHART_AXIS_GRANULARITY);
    x.setGranularityEnabled(true);
    x.setTextColor(ThemeUtils.getColor(mContext, R.attr.elevationProfileAxisLabelColor));
    x.setPosition(XAxis.XAxisPosition.BOTTOM);
    x.setAxisLineColor(ThemeUtils.getColor(mContext, androidx.appcompat.R.attr.dividerHorizontal));
    x.setAxisLineWidth(mContext.getResources().getDimensionPixelSize(R.dimen.divider_height));
    ValueFormatter xAxisFormatter = new AxisValueFormatter(mChart);
    x.setValueFormatter(xAxisFormatter);

    YAxis y = mChart.getAxisLeft();
    y.setLabelCount(CHART_Y_LABEL_COUNT, false);
    y.setPosition(YAxis.YAxisLabelPosition.INSIDE_CHART);
    y.setDrawGridLines(true);
    y.setGridColor(ContextCompat.getColor(mContext, R.color.black_12));
    y.setEnabled(true);
    y.setTextColor(Color.TRANSPARENT);
    y.setAxisLineColor(Color.TRANSPARENT);
    int lineLength = mContext.getResources().getDimensionPixelSize(R.dimen.margin_eighth);
    y.enableGridDashedLine(lineLength, 2 * lineLength, 0);

    mChart.getAxisRight().setEnabled(false);
  }

  public void setData(@Nullable RouteAltitudeData data)
  {
    mData = data;
    if (data == null || data.distances.length == 0)
    {
      mChart.clear();
      mMaxAltitude.setText("");
      mMinAltitude.setText("");
      if (mListener != null)
        mListener.onElevationPointDeselected();
      return;
    }

    List<Entry> values = new ArrayList<>();
    int minAlt = Integer.MAX_VALUE;
    int maxAlt = Integer.MIN_VALUE;

    for (int i = 0; i < data.distances.length; i++)
    {
      float distance = (float) data.distances[i];
      int altitude = data.altitudes[i];
      values.add(new Entry(distance, altitude, i));

      if (altitude < minAlt)
        minAlt = altitude;
      if (altitude > maxAlt)
        maxAlt = altitude;
    }

    LineDataSet set = new LineDataSet(values, ELEVATION_PROFILE_POINTS);
    set.setMode(LineDataSet.Mode.LINEAR);
    set.setDrawFilled(true);
    set.setDrawCircles(false);
    int lineThickness = mContext.getResources().getDimensionPixelSize(R.dimen.divider_width);
    set.setLineWidth(lineThickness);
    int color = ThemeUtils.getColor(mContext, R.attr.elevationProfileColor);
    set.setCircleColor(color);
    set.setColor(color);
    set.setFillAlpha(CHART_FILL_ALPHA);
    set.setFillColor(color);
    set.setDrawHorizontalHighlightIndicator(false);
    set.setHighlightLineWidth(lineThickness);
    set.setHighLightColor(ContextCompat.getColor(mContext, R.color.base_accent_transparent));

    LineData lineData = new LineData(set);
    lineData.setValueTextSize(mContext.getResources().getDimensionPixelSize(R.dimen.text_size_icon_title));
    lineData.setDrawValues(false);

    mChart.setData(lineData);
    mChart.animateX(CHART_ANIMATION_DURATION);

    mMinAltitude.setText(Framework.nativeFormatAltitude(minAlt));
    mMaxAltitude.setText(Framework.nativeFormatAltitude(maxAlt));
  }
}
