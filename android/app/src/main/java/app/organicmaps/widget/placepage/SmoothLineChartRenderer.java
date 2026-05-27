package app.organicmaps.widget.placepage;

import android.graphics.Canvas;
import com.github.mikephil.charting.animation.ChartAnimator;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.highlight.Highlight;
import com.github.mikephil.charting.interfaces.dataprovider.LineDataProvider;
import com.github.mikephil.charting.interfaces.datasets.ILineDataSet;
import com.github.mikephil.charting.renderer.LineChartRenderer;
import com.github.mikephil.charting.utils.MPPointD;
import com.github.mikephil.charting.utils.ViewPortHandler;

public class SmoothLineChartRenderer extends LineChartRenderer
{
  public SmoothLineChartRenderer(LineDataProvider chart, ChartAnimator animator, ViewPortHandler viewPortHandler)
  {
    super(chart, animator, viewPortHandler);
  }

  @Override
  public void drawHighlighted(Canvas c, Highlight[] indices)
  {
    LineData lineData = mChart.getLineData();
    if (lineData == null)
      return;

    for (Highlight high : indices)
    {
      ILineDataSet set = lineData.getDataSetByIndex(high.getDataSetIndex());
      if (set == null || !set.isHighlightEnabled())
        continue;

      // Use the Highlight's own coordinates (interpolated), not the nearest Entry.
      MPPointD pix = mChart.getTransformer(set.getAxisDependency())
                         .getPixelForValues(high.getX(), high.getY() * mAnimator.getPhaseY());

      high.setDraw((float) pix.x, (float) pix.y);
      drawHighlightLines(c, (float) pix.x, (float) pix.y, set);

      MPPointD.recycleInstance(pix);
    }
  }
}
