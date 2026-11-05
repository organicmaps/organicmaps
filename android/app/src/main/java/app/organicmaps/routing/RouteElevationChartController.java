package app.organicmaps.routing;

import android.content.Context;
import android.view.View;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.routing.RouteAltitudeData;
import app.organicmaps.widget.placepage.ElevationChartUtils;
import app.organicmaps.widget.placepage.ElevationProfileChart;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.highlight.Highlight;
import com.github.mikephil.charting.listener.OnChartValueSelectedListener;
import java.util.ArrayList;
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
  @Nullable
  private RouteAltitudeData mData;
  @NonNull
  private List<Entry> mEntries = Collections.emptyList();

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
        if (mListener != null && mData != null)
          mListener.onElevationPointSelected(h.getX());
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
      if (mListener != null && mData != null)
        mListener.onElevationSelectionFinished(x);
    });
  }

  public void setListener(@Nullable ElevationSelectionListener listener)
  {
    mListener = listener;
  }

  public void clearSelection()
  {
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
    return mChart == null ? Double.NaN : mChart.getHighlightX();
  }

  /** Selects a point picked elsewhere without reporting it back to the listener. */
  public void select(double distanceMeters)
  {
    if (mChart == null)
      return;
    // Only a shown chart is selected, and it is shown with data only.
    if (mEntries.isEmpty())
      throw new AssertionError("Selecting on an elevation chart without data");
    mChart.highlightValue(ElevationChartUtils.highlightAt(mEntries, (float) distanceMeters), false);
  }

  public void setData(@Nullable RouteAltitudeData data)
  {
    mData = data;
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
