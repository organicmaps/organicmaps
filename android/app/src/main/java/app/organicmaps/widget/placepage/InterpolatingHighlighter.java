package app.organicmaps.widget.placepage;

import com.github.mikephil.charting.charts.BarLineChartBase;
import com.github.mikephil.charting.components.YAxis;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.github.mikephil.charting.highlight.ChartHighlighter;
import com.github.mikephil.charting.highlight.Highlight;
import com.github.mikephil.charting.utils.MPPointD;
import java.util.List;

/**
 * Replaces the default ChartHighlighter to provide smooth, interpolated
 * highlights instead of snapping to the nearest data point.
 */
public class InterpolatingHighlighter extends ChartHighlighter<BarLineChartBase>
{
  public InterpolatingHighlighter(BarLineChartBase chart)
  {
    super(chart);
  }

  @Override
  public Highlight getHighlight(float x, float y)
  {
    MPPointD pos = getValsForTouch(x, y);
    float xVal = (float) pos.x;
    MPPointD.recycleInstance(pos);

    LineData data = (LineData) mChart.getData();
    if (data == null || data.getDataSetCount() == 0)
      return null;

    LineDataSet set = (LineDataSet) data.getDataSetByIndex(0);
    List<Entry> entries = set.getValues();
    if (entries.isEmpty())
      return null;

    float firstX = entries.get(0).getX();
    float lastX = entries.get(entries.size() - 1).getX();
    if (xVal <= firstX)
      xVal = firstX;
    else if (xVal >= lastX)
      xVal = lastX;

    float yVal = ElevationChartUtils.interpolateY(entries, xVal);

    MPPointD pix = mChart.getTransformer(YAxis.AxisDependency.LEFT).getPixelForValues(xVal, yVal);

    Highlight h = new Highlight(xVal, yVal, (float) pix.x, (float) pix.y, 0, YAxis.AxisDependency.LEFT);

    MPPointD.recycleInstance(pix);
    return h;
  }
}
