package app.organicmaps;

import android.content.Context;
import android.view.View;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.ElevationInfo;
import app.organicmaps.sdk.bookmarks.data.Track;
import app.organicmaps.sdk.bookmarks.data.TrackStatistics;
import app.organicmaps.widget.placepage.CurrentLocationMarkerView;
import app.organicmaps.widget.placepage.ElevationChartUtils;
import app.organicmaps.widget.placepage.FloatingMarkerView;
import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.MarkerView;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.highlight.Highlight;
import com.github.mikephil.charting.listener.OnChartValueSelectedListener;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class ChartController implements OnChartValueSelectedListener
{
  private static final int CURRENT_POSITION_OUT_OF_TRACK = -1;

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
    mChart = view.findViewById(R.id.elevation_profile_chart);

    mFloatingMarkerView = view.findViewById(R.id.floating_marker);
    mCurrentLocationMarkerView = new CurrentLocationMarkerView(mContext);
    mFloatingMarkerView.setChartView(mChart);
    mCurrentLocationMarkerView.setChartView(mChart);

    mMaxAltitude = view.findViewById(R.id.highest_altitude);
    mMinAltitude = view.findViewById(R.id.lowest_altitude);

    ElevationChartUtils.setupChart(mChart, mContext);
    mChart.setOnChartValueSelectedListener(this);
    ElevationChartUtils.initAxes(mChart, mContext, ElevationChartUtils.TRACK_X_LABEL_COUNT);
  }

  public void setData(@Nullable Track track, @NonNull ElevationInfo info, @NonNull TrackStatistics stats)
  {
    mTrack = track;
    List<Entry> values = new ArrayList<>();
    for (ElevationInfo.Point point : info.getPoints())
      values.add(new Entry((float) point.getDistance(), point.getAltitude(), point));

    ElevationChartUtils.setChartData(mChart, values, mContext);

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
    if (mTrack == null)
      return;

    Highlight curPos = getCurrentPosHighlight();

    if (mCurrentPositionOutOfTrack)
      mChart.highlightValues(Collections.singletonList(h), Collections.singletonList(mFloatingMarkerView));
    else
      mChart.highlightValues(Arrays.asList(curPos, h), Arrays.asList(mCurrentLocationMarkerView, mFloatingMarkerView));

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
