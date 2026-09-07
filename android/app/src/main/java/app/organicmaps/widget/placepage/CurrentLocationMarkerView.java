package app.organicmaps.widget.placepage;

import android.annotation.SuppressLint;
import android.content.Context;
import androidx.annotation.NonNull;
import app.organicmaps.R;
import com.github.mikephil.charting.charts.Chart;
import com.github.mikephil.charting.components.MarkerView;
import com.github.mikephil.charting.utils.MPPointF;

@SuppressLint("ViewConstructor")
public class CurrentLocationMarkerView extends MarkerView
{
  /**
   * Constructor. Sets up the MarkerView with a custom layout resource.
   *
   * @param context
   */
  public CurrentLocationMarkerView(@NonNull Context context)
  {
    super(context, R.layout.current_location_marker);
  }

  @Override
  public MPPointF getOffset()
  {
    return new MPPointF(-(getWidth() / 2f), -getHeight());
  }

  @Override
  public MPPointF getOffsetForDrawingAtPoint(float posX, float posY)
  {
    // Clamp horizontally like the base class does, otherwise a label wider than the chart's side
    // offset is cut off at the very start/end of a track. Keep the vertical offset intact to draw
    // the pin above the point instead of pushing it onto the point at peaks.
    MPPointF offset = getOffset();
    Chart chart = getChartView();
    if (posX + offset.x < 0)
      offset.x = -posX;
    else if (chart != null && posX + getWidth() + offset.x > chart.getWidth())
      offset.x = chart.getWidth() - posX - getWidth();
    return offset;
  }
}
