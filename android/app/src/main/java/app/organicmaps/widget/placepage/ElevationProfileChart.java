package app.organicmaps.widget.placepage;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import com.github.mikephil.charting.charts.LineChart;

public class ElevationProfileChart extends LineChart
{
  public ElevationProfileChart(Context context)
  {
    super(context);
  }

  public ElevationProfileChart(Context context, AttributeSet attrs)
  {
    super(context, attrs);
  }

  public ElevationProfileChart(Context context, AttributeSet attrs, int defStyle)
  {
    super(context, attrs, defStyle);
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent ev)
  {
    getParent().requestDisallowInterceptTouchEvent(true);
    return hasZoom() || super.onInterceptTouchEvent(ev);
  }

  private boolean hasZoom()
  {
    return getScaleX() < 1 || getScaleY() < 1;
  }
}
