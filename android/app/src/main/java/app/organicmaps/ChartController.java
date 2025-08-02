package app.organicmaps;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Color;
import android.view.View;
import android.widget.TextView;
import androidx.annotation.NonNull;
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
  private static final float CUBIC_INTENSITY = 0.2f;
  private static final int CURRENT_POSITION_OUT_OF_TRACK = -1;
  private static final String ELEVATION_PROFILE_POINTS = "ELEVATION_PROFILE_POINTS";

  @SuppressWarnings("NullableProblems")
  @NonNull
  private LineChart mChart;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private FloatingMarkerView mFloatingMarkerView;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private MarkerView mCurrentLocationMarkerView;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mMaxAltitude;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mMinAltitude;
  @NonNull
  private final Context mContext;
  private long mTrackId = Utils.INVALID_ID;
  private boolean mCurrentPositionOutOfTrack = true;
  private boolean mInformSelectedActivePointToCore = true;

  public ChartController(@NonNull Context context)
  {
    mContext = context;
  }

  public void initialize(@NonNull View view)
  {
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

  private void highlightChartCurrentLocation()
  {
    mChart.highlightValues(Collections.singletonList(getCurrentPosHighlight()),
                           Collections.singletonList(mCurrentLocationMarkerView));
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

  public void setData(Track track)
  {
    mTrackId = track.getTrackId();
    ElevationInfo info = track.getElevationInfo();
    TrackStatistics stats = track.getTrackStatistics();
    List<Entry> values = new ArrayList<>();

    for (ElevationInfo.Point point : info.getPoints())
      values.add(new Entry((float) point.getDistance(), point.getAltitude(), point));

    LineDataSet set = new LineDataSet(values, ELEVATION_PROFILE_POINTS);
    set.setMode(LineDataSet.Mode.CUBIC_BEZIER);
    set.setCubicIntensity(CUBIC_INTENSITY);
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

    highlightActivePointManually();
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
    if (mTrackId == Utils.INVALID_ID)
      return;

    if (mInformSelectedActivePointToCore)
      BookmarkManager.INSTANCE.setElevationActivePoint(mTrackId, e.getX(), (ElevationInfo.Point) e.getData());
    mInformSelectedActivePointToCore = true;
  }

  @NonNull
  private Highlight getCurrentPosHighlight()
  {
    double activeX = BookmarkManager.INSTANCE.getElevationCurPositionDistance(mTrackId);
    return new Highlight((float) activeX, 0f, 0);
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
    if (mTrackId == Utils.INVALID_ID)
      return;

    double distance = BookmarkManager.INSTANCE.getElevationCurPositionDistance(mTrackId);
    mCurrentPositionOutOfTrack = distance == CURRENT_POSITION_OUT_OF_TRACK;
    highlightActivePointManually();
  }

  public void onElevationActivePointChanged()
  {
    if (mTrackId == Utils.INVALID_ID)
      return;

    highlightActivePointManually();
  }

  private void highlightActivePointManually()
  {
    Highlight highlight = getActivePoint();
    mInformSelectedActivePointToCore = false;
    mChart.highlightValue(highlight, true);
  }

  @NonNull
  private Highlight getActivePoint()
  {
    double activeX = BookmarkManager.INSTANCE.getElevationActivePointDistance(mTrackId);
    return new Highlight((float) activeX, 0f, 0);
  }

  public void onHide()
  {
    mChart.fitScreen();
    mTrackId = Utils.INVALID_ID;
  }
}
