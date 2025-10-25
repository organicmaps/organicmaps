package app.organicmaps;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Color;
import android.view.View;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.ElevationInfo;
import app.organicmaps.sdk.bookmarks.data.Track;
import app.organicmaps.sdk.bookmarks.data.TrackStatistics;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.placepage.AxisValueFormatter;
import app.organicmaps.widget.placepage.CurrentLocationMarkerView;
import app.organicmaps.widget.placepage.FloatingMarkerView;
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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class ChartController implements OnChartValueSelectedListener
{
  private static final int CHART_Y_LABEL_COUNT = 3;
  private static final int CHART_X_LABEL_COUNT = 6;
  private static final int CHART_ANIMATION_DURATION = 0;
  private static final int CHART_FILL_ALPHA = (int) (0.12 * 255);
  private static final int CHART_AXIS_GRANULARITY = 100;
  private static final int CURRENT_POSITION_OUT_OF_TRACK = -1;
  private static final String ELEVATION_PROFILE_POINTS = "ELEVATION_PROFILE_POINTS";

  @NonNull
  private final Context mContext;
  @NonNull
  private final LineChart mChart;
  @NonNull
  private final FloatingMarkerView mFloatingMarkerView;
  @NonNull
  private final MarkerView mCurrentLocationMarkerView;
  @NonNull
  private final TextView mMaxAltitude;
  @NonNull
  private final TextView mMinAltitude;

  @Nullable
  private Track mTrack;

  private boolean mCurrentPositionOutOfTrack = true;
  private boolean mInformSelectedActivePointToCore = true;

  public ChartController(@NonNull View view)
  {
    mContext = view.getContext();
    final Resources resources = mContext.getResources();
    mChart = view.findViewById(R.id.elevation_profile_chart);

    mFloatingMarkerView = view.findViewById(R.id.floating_marker);
    mCurrentLocationMarkerView = new CurrentLocationMarkerView(mContext);
    mFloatingMarkerView.setChartView(mChart);
    mCurrentLocationMarkerView.setChartView(mChart);

    mMaxAltitude = view.findViewById(R.id.highest_altitude);
    mMinAltitude = view.findViewById(R.id.lowest_altitude);

    mChart.setBackgroundColor(ThemeUtils.getColor(mContext, R.attr.cardBackground));
    mChart.setTouchEnabled(true);
    mChart.setOnChartValueSelectedListener(this);
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
    initAxises();
  }

  private void initAxises()
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

  public void setData(@Nullable Track track, @NonNull ElevationInfo info, @NonNull TrackStatistics stats)
  {
    mTrack = track;
    List<Entry> values = new ArrayList<>();
    for (ElevationInfo.Point point : info.getPoints())
      values.add(new Entry((float) point.getDistance(), point.getAltitude(), point));

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

    LineData data = new LineData(set);
    data.setValueTextSize(mContext.getResources().getDimensionPixelSize(R.dimen.text_size_icon_title));
    data.setDrawValues(false);

    mChart.setData(data);
    mChart.animateX(CHART_ANIMATION_DURATION);

    mMinAltitude.setText(Framework.nativeFormatAltitude(stats.getMinElevation()));
    mMaxAltitude.setText(Framework.nativeFormatAltitude(stats.getMaxElevation()));

    if (track != null)
      highlightActivePointManually();
    mChart.setTouchEnabled(mTrack != null);
  }

  @Override
  public void onValueSelected(Entry e, Highlight h)
  {
    mFloatingMarkerView.updateOffsets(e, h);
    Highlight curPos = getCurrentPosHighlight();

    if (mCurrentPositionOutOfTrack)
      mChart.highlightValues(Collections.singletonList(h), Collections.singletonList(mFloatingMarkerView));
    else
      mChart.highlightValues(Arrays.asList(curPos, h), Arrays.asList(mCurrentLocationMarkerView, mFloatingMarkerView));
    if (mTrack == null)
      return;

    if (mInformSelectedActivePointToCore)
      BookmarkManager.INSTANCE.setElevationActivePoint(mTrack.getTrackId(), e.getX(),
                                                       (ElevationInfo.Point) e.getData());
    mInformSelectedActivePointToCore = true;
  }

  @NonNull
  private Highlight getCurrentPosHighlight()
  {
    return new Highlight((float) mTrack.getElevationCurPositionDistance(), 0f, 0);
  }

  @Override
  public void onNothingSelected()
  {
    if (mCurrentPositionOutOfTrack)
      return;

    highlightChartCurrentLocation();
  }

  public void onCurrentPositionChanged()
  {
    if (mTrack == null)
      return;

    final double distance = mTrack.getElevationCurPositionDistance();
    mCurrentPositionOutOfTrack = distance == CURRENT_POSITION_OUT_OF_TRACK;
    highlightActivePointManually();
  }

  public void onElevationActivePointChanged()
  {
    if (mTrack == null)
      return;

    highlightActivePointManually();
  }

  private void highlightActivePointManually()
  {
    Highlight highlight = getActivePoint();
    mInformSelectedActivePointToCore = false;
    mChart.highlightValue(highlight, true);
  }

  private void highlightChartCurrentLocation()
  {
    mChart.highlightValues(Collections.singletonList(getCurrentPosHighlight()),
                           Collections.singletonList(mCurrentLocationMarkerView));
  }

  @NonNull
  private Highlight getActivePoint()
  {
    double activeX = mTrack.getElevationActivePointDistance();
    return new Highlight((float) activeX, 0f, 0);
  }
}
