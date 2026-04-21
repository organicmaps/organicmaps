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
import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.highlight.Highlight;
import com.github.mikephil.charting.listener.OnChartValueSelectedListener;
import java.util.ArrayList;
import java.util.List;

public class RouteElevationChartController
{
  @NonNull
  private final Context mContext;
  @Nullable
  private final LineChart mChart;
  @Nullable
  private final TextView mMaxAltitude;
  @Nullable
  private final TextView mMinAltitude;

  @Nullable
  private ElevationSelectionListener mListener;
  @Nullable
  private RouteAltitudeData mData;

  public interface ElevationSelectionListener
  {
    void onElevationPointSelected(double distanceMeters);
    void onElevationPointDeselected();
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

  public void setData(@Nullable RouteAltitudeData data)
  {
    mData = data;
    if (mChart == null)
      return;

    if (data == null || data.getSize() == 0)
    {
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

    ElevationChartUtils.configureYAxisBounds(mChart, data.getMinAltitude(), data.getMaxAltitude());
    ElevationChartUtils.setChartData(mChart, values, mContext);

    if (mMinAltitude != null)
      mMinAltitude.setText(Framework.nativeFormatAltitude(data.getMinAltitude()));
    if (mMaxAltitude != null)
      mMaxAltitude.setText(Framework.nativeFormatAltitude(data.getMaxAltitude()));
  }
}
