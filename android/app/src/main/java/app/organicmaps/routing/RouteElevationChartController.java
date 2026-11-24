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
    mChart.highlightValue(ElevationChartUtils.highlightAt(mEntries, (float) distanceMeters), false);
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

    List<Entry> values = new ArrayList<>();
    for (int i = 0; i < data.getSize(); i++)
      values.add(new Entry((float) data.getDistance(i), data.getAltitude(i), i));
    mEntries = values;

    ElevationChartUtils.configureYAxisBounds(mChart, data.getMinAltitude(), data.getMaxAltitude());
    ElevationChartUtils.setChartData(mChart, values, mContext);

    if (mMinAltitude != null)
      mMinAltitude.setText(Framework.nativeFormatAltitude(data.getMinAltitude()));
    if (mMaxAltitude != null)
      mMaxAltitude.setText(Framework.nativeFormatAltitude(data.getMaxAltitude()));
  }
}
