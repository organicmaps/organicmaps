package com.mapswithme.maps;

import android.content.res.Resources;
import android.graphics.Color;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.Legend;
import com.github.mikephil.charting.components.MarkerView;
import com.github.mikephil.charting.components.XAxis;
import com.github.mikephil.charting.components.YAxis;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.github.mikephil.charting.formatter.ValueFormatter;
import com.github.mikephil.charting.highlight.Highlight;
import com.github.mikephil.charting.listener.OnChartValueSelectedListener;
import com.mapswithme.maps.widget.placepage.AxisValueFormatter;
import com.mapswithme.maps.widget.placepage.CurrentLocationMarkerView;
import com.mapswithme.maps.widget.placepage.FloatingMarkerView;
import com.mapswithme.util.ThemeUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

class ChartController implements OnChartValueSelectedListener
{
  private static final int CHART_Y_LABEL_COUNT = 3;
  private static final int CHART_X_LABEL_COUNT = 6;
  private static final int CHART_ANIMATION_DURATION = 1500;
  private static final int CHART_FILL_ALPHA = (int) (0.12 * 255);
  private static final float CUBIC_INTENSITY = 0.2f;

  @NonNull
  private final AppCompatActivity mActivity;

  @NonNull
  private final LineChart mChart;

  @NonNull
  private final FloatingMarkerView mFloatingMarkerView;

  @NonNull
  private final MarkerView mCurrentLocationMarkerView;

  ChartController(@NonNull AppCompatActivity activity)
  {
    mActivity = activity;
    mChart = getActivity().findViewById(R.id.elevation_profile_chart);
    mFloatingMarkerView = new FloatingMarkerView(getActivity());
    mCurrentLocationMarkerView = new CurrentLocationMarkerView(getActivity());
    mFloatingMarkerView.setChartView(mChart);
    mCurrentLocationMarkerView.setChartView(mChart);
    initChart();
  }

  @NonNull
  private AppCompatActivity getActivity()
  {
    return mActivity;
  }

  private void initChart()
  {
    TextView topAlt = getActivity().findViewById(R.id.highest_altitude);
    topAlt.setText("10000m");
    TextView bottomAlt = getActivity().findViewById(R.id.lowest_altitude);
    bottomAlt.setText("100m");

    mChart.setBackgroundColor(ThemeUtils.getColor(getActivity(), android.R.attr.textColorPrimaryInverse));
    mChart.setTouchEnabled(true);
    mChart.setOnChartValueSelectedListener(this);
    mChart.setDrawGridBackground(false);
    mChart.setDragEnabled(true);
    mChart.setScaleEnabled(true);
    mChart.setPinchZoom(true);
    mChart.setExtraTopOffset(0);
    int sideOffset = getResources().getDimensionPixelSize(R.dimen.margin_base);
    int topOffset = 0;
    mChart.setViewPortOffsets(sideOffset, topOffset, sideOffset, getResources().getDimensionPixelSize(R.dimen.margin_base_plus_quarter));
    mChart.getDescription().setEnabled(false);
    mChart.setDrawBorders(false);
    Legend l = mChart.getLegend();
    l.setEnabled(false);
    initAxises();
    setData(20, 180);


    highlightChartCurrentLocation();
    mChart.animateX(CHART_ANIMATION_DURATION);
  }

  @NonNull
  private Resources getResources()
  {
    return getActivity().getResources();
  }

  private void highlightChartCurrentLocation()
  {
    mChart.highlightValues(Collections.singletonList(getCurrentPosHighlight()),
                           Collections.singletonList(mCurrentLocationMarkerView));
  }

  private void initAxises()
  {
    XAxis x = mChart.getXAxis();
    x.setLabelCount(CHART_X_LABEL_COUNT, false);
    x.setAvoidFirstLastClipping(true);
    x.setDrawGridLines(false);
    x.setTextColor(ThemeUtils.getColor(getActivity(), R.attr.elevationProfileAxisLabelColor));
    x.setPosition(XAxis.XAxisPosition.BOTTOM);
    x.setAxisLineColor(ThemeUtils.getColor(getActivity(), R.attr.dividerHorizontal));
    x.setAxisLineWidth(getActivity().getResources().getDimensionPixelSize(R.dimen.divider_height));
    ValueFormatter xAxisFormatter = new AxisValueFormatter();
    x.setValueFormatter(xAxisFormatter);

    YAxis y = mChart.getAxisLeft();
    y.setLabelCount(CHART_Y_LABEL_COUNT, false);
    y.setPosition(YAxis.YAxisLabelPosition.INSIDE_CHART);
    y.setDrawGridLines(true);
    y.setGridColor(getResources().getColor(R.color.black_12));
    y.setEnabled(true);
    y.setTextColor(Color.TRANSPARENT);
    y.setAxisLineColor(Color.TRANSPARENT);
    int lineLength = getResources().getDimensionPixelSize(R.dimen.margin_eighth);
    y.enableGridDashedLine(lineLength, 2 * lineLength, 0);

    mChart.getAxisRight().setEnabled(false);
  }

  private void setData(int count, float range)
  {
    List<Entry> values = new ArrayList<>();

    for (int i = 0; i < count; i++)
    {
      float val = (float) (Math.random() * (range + 1)) + 20;
      values.add(new Entry(i, val));
    }

    LineDataSet set;

    // create a dataset and give it a type
    set = new LineDataSet(values, "DataSet 1");

    set.setMode(LineDataSet.Mode.CUBIC_BEZIER);
    set.setCubicIntensity(CUBIC_INTENSITY);
    set.setDrawFilled(true);
    set.setDrawCircles(false);
    int lineThickness = getResources().getDimensionPixelSize(R.dimen.divider_width);
    set.setLineWidth(lineThickness);
    set.setCircleColor(getResources().getColor(R.color.base_accent));
    set.setColor(getResources().getColor(R.color.base_accent));
    set.setFillAlpha(CHART_FILL_ALPHA);
    set.setDrawHorizontalHighlightIndicator(false);
    set.setHighlightLineWidth(lineThickness);
    set.setHighLightColor(getResources().getColor(R.color.base_accent_transparent));
    set.setFillColor(getResources().getColor(R.color.elevation_profile));

    LineData data = new LineData(set);
    data.setValueTextSize(getResources().getDimensionPixelSize(R.dimen.text_size_icon_title));
    data.setDrawValues(false);

    mChart.setData(data);
  }


  @Override
  public void onValueSelected(Entry e, Highlight h)
  {
    mFloatingMarkerView.updateOffsets(e, h);
    Highlight curPos = getCurrentPosHighlight();

    mChart.highlightValues(Arrays.asList(curPos, h),
                           Arrays.asList(mCurrentLocationMarkerView, mFloatingMarkerView));
  }

  @NonNull
  private Highlight getCurrentPosHighlight()
  {
    LineData data = mChart.getData();
    final Entry entryForIndex = data.getDataSetByIndex(0).getEntryForIndex(5);
    return new Highlight(entryForIndex.getX(), 0, 5);
  }

  @Override
  public void onNothingSelected()
  {
    highlightChartCurrentLocation();
  }
}
