package app.organicmaps.routing;

import android.content.Context;
import android.view.View;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.routing.RouteAltitudeData;
import app.organicmaps.sdk.util.Assert;
import app.organicmaps.widget.placepage.ElevationChartUtils;
import app.organicmaps.widget.placepage.ElevationProfileChart;
import app.organicmaps.widget.placepage.SmoothLineChartRenderer.FollowedSection;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.highlight.Highlight;
import com.github.mikephil.charting.listener.OnChartValueSelectedListener;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class RouteElevationChartController
{
  @NonNull
  private final Context mContext;
  @Nullable
  private final ElevationProfileChart mChart;
  @Nullable
  private final TextView mMaxAltitude;
  @Nullable
  private final TextView mMinAltitude;

  @Nullable
  private ElevationSelectionListener mListener;
  // The exact distances of the chart points, or null without data.
  @Nullable
  private double[] mDistances;
  @NonNull
  private List<Entry> mEntries = Collections.emptyList();
  // The exact distance of the last select(): the chart highlight keeps only its float.
  private double mSelectedDistance = Double.NaN;

  public interface ElevationSelectionListener
  {
    void onElevationPointSelected(double distanceMeters);
    void onElevationPointDeselected();
    void onElevationSelectionFinished(double distanceMeters);
  }

  public RouteElevationChartController(@NonNull View view)
  {
    mContext = view.getContext();
    mChart = view.findViewById(R.id.elevation_profile_chart);
    mMaxAltitude = view.findViewById(R.id.highest_altitude);
    mMinAltitude = view.findViewById(R.id.lowest_altitude);

    if (mChart == null)
      return;

    ElevationChartUtils.setupRouteChart(mChart, mContext);
    mChart.setOnChartValueSelectedListener(new OnChartValueSelectedListener() {
      @Override
      public void onValueSelected(Entry e, Highlight h)
      {
        if (mListener != null && mDistances != null)
          mListener.onElevationPointSelected(toDistance(h.getX()));
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
    mChart.setOnSelectionFinishedListener(x -> {
      if (mListener != null && mDistances != null)
        mListener.onElevationSelectionFinished(toDistance(x));
    });
  }

  private double toDistance(float chartX)
  {
    return mDistances == null ? chartX : ElevationChartUtils.toDistance(chartX, mSelectedDistance, mDistances);
  }

  public void setListener(@Nullable ElevationSelectionListener listener)
  {
    mListener = listener;
  }

  public void clearSelection()
  {
    mSelectedDistance = Double.NaN;
    if (mChart != null)
      mChart.highlightValue(null, false);
  }

  public void setZoomEnabled(boolean enabled)
  {
    if (mChart != null)
      mChart.setScaleXEnabled(enabled);
  }

  public boolean isUserSelecting()
  {
    return mChart != null && mChart.isSelectionInProgress();
  }

  /** @return the selected distance, or NaN without a selection. */
  public double getSelectedDistance()
  {
    return mChart == null ? Double.NaN : toDistance(mChart.getHighlightX());
  }

  /**
   * Selects a point picked elsewhere without reporting it back to the listener. The chart reports this point with
   * the exact distance again.
   */
  public void select(double distanceMeters)
  {
    if (mChart == null)
      return;
    // Only a shown chart is selected, and it is shown with data only.
    Assert.debug(!mEntries.isEmpty(), "Selecting on an elevation chart without data");
    mSelectedDistance = distanceMeters;
    final float x = (float) distanceMeters;
    final Entry entry = FollowedSection.entryAt(mChart.getData(), x);
    mChart.highlightValue(
        entry == null ? ElevationChartUtils.highlightAt(mEntries, x) : new Highlight(x, entry.getY(), 0), false);
  }

  public void setData(@Nullable RouteAltitudeData data)
  {
    final double[] distances = data == null ? null : data.getDistances();
    // A replay of the shown profile keeps its selection. Another route must not inherit it: the selection is put back
    // on the map, and the chart keeps its highlight across new data. Its map marker would point into the old route.
    if (!Arrays.equals(distances, mDistances))
    {
      final boolean selected = mChart != null && !Float.isNaN(mChart.getHighlightX());
      clearSelection();
      if (selected && mListener != null)
        mListener.onElevationPointDeselected();
    }
    mDistances = distances;
    if (mChart == null)
      return;

    if (data == null || data.getSize() == 0)
    {
      mEntries = Collections.emptyList();
      mChart.clear();
      if (mMaxAltitude != null)
        mMaxAltitude.setText("");
      if (mMinAltitude != null)
        mMinAltitude.setText("");
      return;
    }

    List<Entry> values = new ArrayList<>(data.getSize());
    int chartMin = data.getAltitude(0);
    int chartMax = chartMin;
    for (int i = 0; i < data.getSize(); i++)
    {
      final int altitude = data.getAltitude(i);
      values.add(new Entry((float) data.getDistance(i), altitude, i));
      chartMin = Math.min(chartMin, altitude);
      chartMax = Math.max(chartMax, altitude);
    }
    mEntries = values;

    // The full chart also shows elevations outside the followed section.
    ElevationChartUtils.configureYAxisBounds(mChart, chartMin, chartMax);
    ElevationChartUtils.setChartData(mChart, values, mContext, followedSection(data, values));

    if (mMinAltitude != null)
      mMinAltitude.setText(Framework.nativeFormatAltitude(chartMin));
    if (mMaxAltitude != null)
      mMaxAltitude.setText(Framework.nativeFormatAltitude(chartMax));
  }

  @Nullable
  private static FollowedSection followedSection(RouteAltitudeData data, List<Entry> values)
  {
    if (data.finishPointIndex < 0)
      return null;
    final double index = data.entryPointIndex;
    final int lower = (int) Math.floor(index);
    final int upper = (int) Math.ceil(index);
    final double fraction = index - lower;
    final Entry entry =
        new Entry((float) (data.getDistance(lower) + fraction * (data.getDistance(upper) - data.getDistance(lower))),
                  (float) (data.getAltitude(lower) + fraction * (data.getAltitude(upper) - data.getAltitude(lower))));
    final int finish = data.finishPointIndex;
    final boolean reversed = finish < index;
    final List<Entry> section = new ArrayList<>(Math.abs(finish - lower) + 2);
    if (reversed)
    {
      section.addAll(values.subList(finish, upper));
      section.add(entry);
    }
    else
    {
      section.add(entry);
      section.addAll(values.subList(lower + 1, finish + 1));
    }
    return new FollowedSection(section, reversed);
  }
}
