package app.organicmaps.widget.placepage;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import androidx.annotation.Nullable;
import com.github.mikephil.charting.animation.ChartAnimator;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.github.mikephil.charting.highlight.Highlight;
import com.github.mikephil.charting.interfaces.dataprovider.LineDataProvider;
import com.github.mikephil.charting.interfaces.datasets.ILineDataSet;
import com.github.mikephil.charting.renderer.LineChartRenderer;
import com.github.mikephil.charting.utils.MPPointD;
import com.github.mikephil.charting.utils.Utils;
import com.github.mikephil.charting.utils.ViewPortHandler;
import java.util.List;

public class SmoothLineChartRenderer extends LineChartRenderer
{
  public static final class FollowedSection extends LineDataSet
  {
    private final boolean reversed;

    public FollowedSection(List<Entry> points, boolean reversed)
    {
      super(points, null);
      this.reversed = reversed;
    }

    // At a gap the accepted entry identifies which of the two heights to highlight.
    @Nullable
    public static Entry entryAt(LineData data, float x)
    {
      if (data.getDataSetByIndex(1) instanceof FollowedSection section && section.getEntryPoint().getX() == x)
        return section.getEntryPoint();
      return null;
    }

    private Entry getEntryPoint()
    {
      return getEntryForIndex(reversed ? getEntryCount() - 1 : 0);
    }
  }

  private final Path mDirectionArrow = new Path();

  public SmoothLineChartRenderer(LineDataProvider chart, ChartAnimator animator, ViewPortHandler viewPortHandler)
  {
    super(chart, animator, viewPortHandler);
  }

  @Override
  protected void drawDataSet(Canvas canvas, ILineDataSet dataSet)
  {
    super.drawDataSet(canvas, dataSet);
    if (!(dataSet instanceof FollowedSection section))
      return;

    final var transformer = mChart.getTransformer(section.getAxisDependency());
    final MPPointD entry = transformer.getPixelForValues(section.getEntryPoint().getX(), 0);
    final MPPointD finish = transformer.getPixelForValues(section.reversed ? section.getXMin() : section.getXMax(), 0);
    final float x = (float) entry.x;
    final float delta = (float) (finish.x - entry.x);
    MPPointD.recycleInstance(entry);
    MPPointD.recycleInstance(finish);

    final float sign = Math.signum(delta);
    final float length = Math.min(Utils.convertDpToPixel(18), Math.abs(delta));
    final float tip = x + sign * length;
    final float head = Math.min(Utils.convertDpToPixel(4), length / 2);
    final float y = mViewPortHandler.contentTop() + Utils.convertDpToPixel(8);
    mDirectionArrow.rewind();
    mDirectionArrow.moveTo(x, y);
    mDirectionArrow.lineTo(tip, y);
    mDirectionArrow.moveTo(tip - sign * head, y - head);
    mDirectionArrow.lineTo(tip, y);
    mDirectionArrow.lineTo(tip - sign * head, y + head);
    mRenderPaint.setStyle(Paint.Style.STROKE);
    mRenderPaint.setStrokeWidth(Utils.convertDpToPixel(2));
    mRenderPaint.setColor(section.getColor());
    canvas.drawPath(mDirectionArrow, mRenderPaint);
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
